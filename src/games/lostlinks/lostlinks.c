/* LOST LINKS - a golf ball's adventure across the links and under them.
 * Cartridge 18 of UFO 40, a tribute to Golfaria (UFO 50 #18). Every rule
 * and where it comes from is in docs/games/18-lost-links.md.
 *
 * This file is the cartridge: the title, a run from the last pin, strokes,
 * the things in the world, the finale and the Brass Badger, drawing, saving
 * and the test hooks (with the demo player). How the ball rolls is in
 * lostlinks_ball.c, the planning in lostlinks_probe.c. */
#include "lostlinks.h"

#define START_STROKES 20   /* [QB], [LIZ-18]: twenty to start */
#define IRON_STROKES 3     /* each iron: three more, and a refill */
#define SAFE_TOP 15        /* a hole by a pin tops you up to fifteen */
#define CROW_WATCH 5       /* a scorecrow flies off after five strokes */
#define CROW_RANGE 112     /* seven tiles */
#define LARK_STROKES 4
#define ALBA_STROKES 8
#define SLICER_STROKES 5
#define STRAY_HELP 8       /* a stray's help in the finale */
#define BADGER_HP 6        /* "five or six hits to the face" */
#define BADGER_SHOTS 6     /* "you'll have six shots to get him again" */
#define VOLLEYS 3
#define VOLLEY_T 150
#define VOLLEY_COST 3
#define TOUCH_R 9.0f

enum { ST_TITLE, ST_INTRO, ST_PLAY, ST_TALK, ST_WARP, ST_OUT, ST_END };
enum { SL_IDLE, SL_CHASE, SL_BACK, SL_KNOCKED, SL_GONE };
enum { CR_PERCH, CR_WATCH, CR_FLOWN, CR_DEAD };
enum { BD_SLEEP, BD_UP, BD_STAND, BD_FIRE, BD_DIG, BD_DEAD };

typedef struct Save {
    uint32_t magic;
    uint32_t irons;
    uint16_t crows, pins;
    uint8_t strays, abilities, pieces, checkpoint; /* checkpoint: a pin id, 0xFF = where Dimple woke */
    uint8_t placed, won, won_all, secrets;
    uint32_t strokes_total;
    uint16_t runs, wins;
    uint16_t best_strokes;  /* fewest strokes in a winning game (0 = none) */
    uint16_t pad;
} Save;
#define SAVE_MAGIC 0x4C4E4B02u

static Save sv;
static bool has_save;
static uint8_t tiles[LNK_LAYERS][LNK_MH][LNK_MW];
static LnkCtx ctx;
static LnkBall ball;
static int state, state_t, frame_t, title_sel, title_confirm;
static int strokes, aim = 0, meter_hold, level_shown;
static float meter_p;
static int meter_dir;
static bool charging, panning;
static int b_held, pan_x, pan_y, aim_hold;
static float cam_x, cam_y;
static int banner_t, banner_col, last_zone = -1;
static char banner[48];
static int warp_t;
static int unsteady_t, pop_cool;
static int shake_t;
static int stroke_run;        /* strokes this game (for the best) */
static int intro_i, end_i;

/* dialog */
static char talk_buf[256];
static const char *talk_title;
static int talk_after;         /* what happens when it closes */
enum { TA_NONE, TA_OUT_CHECK, TA_END };

/* the moving things */
typedef struct Mover {
    uint8_t kind, layer, state, id;
    float x, y, hx, hy, vx, vy;
    int t;
} Mover;
static Mover movers[96];
static int n_movers;
static bool touch_armed[LNK_MAX_THINGS];
static uint8_t plates_lit, strays_used;
static int crow_countdown[LNK_CROWS];

/* the Brass Badger */
static struct {
    int state, t, hp, spot, shots_left, volley;
    float cx, cy;              /* the crosshair */
    bool cross_on;
} bd;
static const int BURROW[5][2] = {{10, 6}, {7, 3}, {14, 3}, {6, 8}, {15, 8}};

/* particles */
typedef struct { float x, y, vx, vy; int life, col; } Part;
static Part parts[96];

/* test hooks */
static int bot_wp, bot_hop_at, bot_brake_at, bot_shots, bot_fail;
static bool bot_flow_ok, bot_have;
static LnkShot bot_shot;
static LnkFlow bot_flow;
static uint8_t bot_flow_ab, bot_flow_open;
static int bot_flow_wp = -1;
static bool demo_seen_npc[LNK_MAX_THINGS];

static void part_add(float x, float y, float vx, float vy, int life, int col) {
    for (int i = 0; i < ARRAY_LEN(parts); i++)
        if (parts[i].life <= 0) { parts[i] = (Part){x, y, vx, vy, life, col}; return; }
}

static void burst(float x, float y, int col, int n, float sp) {
    for (int i = 0; i < n; i++) {
        float a = (float)i / (float)n * 6.283f;
        part_add(x, y, cosf(a) * sp, sinf(a) * sp, 14 + i % 7, col);
    }
}

static int popcount32(uint32_t v) {
    int n = 0;
    while (v) { n += v & 1; v >>= 1; }
    return n;
}

static int max_strokes(void) { return START_STROKES + IRON_STROKES * popcount32(sv.irons); }
static bool has_ab(int ab) { return (sv.abilities >> ab) & 1; }

static void sync_ctx(void) {
    ctx.tiles = tiles;
    ctx.abilities = sv.abilities;
    ctx.probe = false;
    ctx.opened = (uint8_t)((sv.placed ? 1 : 0) | (plates_lit == 0xF ? 2 : 0));
}

static void save_now(void) {
    sv.magic = SAVE_MAGIC;
    game_save_write(game_current_index(), &sv, (int)sizeof sv);
    has_save = true;
}

static bool load_save(void) {
    Save tmp;
    if (game_save_read(game_current_index(), &tmp, (int)sizeof tmp) != (int)sizeof tmp || tmp.magic != SAVE_MAGIC) return false;
    sv = tmp;
    return true;
}

static bool all_done(void) {
    return popcount32(sv.irons) == LNK_IRONS && popcount32(sv.crows) == LNK_CROWS && popcount32(sv.strays) == LNK_STRAYS &&
           sv.abilities == 0xF && sv.pieces == 0xF;
}

/* ------------------------------------------------------------------ */
/* the run                                                              */

static const LnkThing *thing_at(int i) { return &lnk_things[i]; }

static float tcx(const LnkThing *t) { return t->tx * LNK_T + LNK_T / 2.0f; }
static float tcy(const LnkThing *t) { return t->ty * LNK_T + LNK_T / 2.0f; }

static void spawn_movers(void) {
    n_movers = 0;
    for (int i = 0; i < lnk_nthings && n_movers < ARRAY_LEN(movers); i++) {
        const LnkThing *t = thing_at(i);
        if (t->kind != EK_SLICER && t->kind != EK_LARK && t->kind != EK_ALBA && t->kind != EK_CROW) continue;
        Mover *m = &movers[n_movers++];
        memset(m, 0, sizeof *m);
        m->kind = t->kind;
        m->layer = t->layer;
        m->id = t->id;
        m->x = m->hx = tcx(t);
        m->y = m->hy = tcy(t);
        m->t = i * 17;
        if (t->kind == EK_CROW) {
            m->state = (sv.crows >> t->id) & 1 ? CR_DEAD : CR_PERCH;
            crow_countdown[t->id] = CROW_WATCH;
        }
    }
}

static void place_ball(int layer, float x, float y) {
    memset(&ball, 0, sizeof ball);
    ball.layer = ball.slayer = (uint8_t)layer;
    ball.x = ball.sx = x;
    ball.y = ball.sy = y;
    ball.lie = LIE_GROUND;
}

static void checkpoint_spot(int *layer, float *x, float *y) {
    const LnkThing *t = sv.checkpoint == 0xFF ? lnk_find(EK_START, 0) : lnk_find(EK_PIN, sv.checkpoint);
    if (!t) t = lnk_find(EK_START, 0);
    *layer = t->layer;
    *x = tcx(t);
    *y = tcy(t) + (t->kind == EK_PIN ? 4.0f : 0.0f);
}

/* a new run from the last pin: everything but what you keep comes back */
static void start_run(void) {
    lnk_tiles_reset(tiles);
    plates_lit = 0;
    strays_used = 0;
    sync_ctx();
    spawn_movers();
    int l;
    float x, y;
    checkpoint_spot(&l, &x, &y);
    place_ball(l, x, y);
    strokes = max_strokes();
    charging = panning = false;
    meter_p = 0;
    for (int i = 0; i < lnk_nthings; i++) touch_armed[i] = true;
    /* don't talk to the pin you are standing by */
    for (int i = 0; i < lnk_nthings; i++) {
        const LnkThing *t = thing_at(i);
        float dx = tcx(t) - ball.x, dy = tcy(t) - ball.y;
        if (t->layer == ball.layer && dx * dx + dy * dy < 30 * 30) touch_armed[i] = false;
    }
    memset(&bd, 0, sizeof bd);
    bd.state = BD_SLEEP;
    bd.hp = BADGER_HP;
    cam_x = ball.x - SCREEN_W / 2;
    cam_y = ball.y - SCREEN_H / 2;
    last_zone = -1;
    sv.runs++;
    state = ST_PLAY;
    state_t = 0;
}

static void new_game(void) {
    memset(&sv, 0, sizeof sv);
    sv.checkpoint = 0xFF;
    stroke_run = 0;
    save_now();
    start_run();
    state = ST_INTRO;
    intro_i = 0;
    state_t = 0;
}

static void talk(const char *title, const char *text, int after) {
    talk_title = title;
    snprintf(talk_buf, sizeof talk_buf, "%s", text);
    talk_after = after;
    state = ST_TALK;
    state_t = 0;
}

static void gain_strokes(int n) {
    strokes = imin(max_strokes(), strokes + n);
}

static void out_of_strokes(void) {
    state = ST_OUT;
    state_t = 0;
    music_play(LNK_MUS_OUT);
}

/* after the ball stops (or sinks) with no strokes left */
static void check_out(void) {
    if (strokes <= 0 && !ball.moving && ball.lie != LIE_CUP && state == ST_PLAY) out_of_strokes();
}

static bool in_finale(void) {
    /* the sanctum and the den (the underground's top-left two zones) */
    return ball.layer == LNK_UNDER && ball.y < LNK_ZH * LNK_T && ball.x < 2 * LNK_ZW * LNK_T;
}

/* in the finale each rescued stray gives a hand once, when strokes run low */
static void stray_help(void) {
    if (!in_finale() || strokes > 2) return;
    int have = popcount32(sv.strays);
    if (strays_used >= have) return;
    int k = 0, which = -1;
    for (int i = 0; i < LNK_STRAYS; i++)
        if ((sv.strays >> i) & 1) { if (k == strays_used) { which = i; break; } k++; }
    strays_used++;
    gain_strokes(STRAY_HELP);
    char buf[96];
    snprintf(buf, sizeof buf, "%s ROLLS IN TO HELP! +%d STROKES.", which >= 0 ? LNK_STRAY_NAME[which] : "A STRAY", STRAY_HELP);
    sfx_play_name("lnk_help");
    talk(NULL, buf, TA_NONE);
}

static void take_stroke(bool free_chip) {
    if (!free_chip) {
        strokes--;
        sv.strokes_total++;
        stroke_run++;
        /* every scorecrow that is watching counts it */
        for (int i = 0; i < n_movers; i++) {
            Mover *m = &movers[i];
            if (m->kind != EK_CROW || m->state != CR_WATCH) continue;
            if (--crow_countdown[m->id] <= 0) {
                m->state = CR_FLOWN;
                m->t = 0;
                sfx_play_name("lnk_flap");
            }
        }
        if (bd.state == BD_UP && --bd.shots_left <= 0) {
            bd.state = BD_STAND;
            bd.t = 0;
        }
    }
}

static void shoot(void) {
    bool free_chip = ball.lie == LIE_CUP, chip = lnk_is_chip(&ball, &ctx);
    lnk_hit(&ball, &ctx, aim, level_shown);
    take_stroke(free_chip);
    charging = false;
    unsteady_t = 0;
    sfx_play_name(chip ? "lnk_chip" : "lnk_hit");
}

/* ---- touching things ------------------------------------------------ */

static void got_item(const char *what, const char *line) {
    music_play(LNK_MUS_ITEM);
    strokes = max_strokes();
    save_now();
    talk(what, line, TA_NONE);
}

static void touch_thing(int i) {
    const LnkThing *t = thing_at(i);
    char buf[200];
    switch (t->kind) {
    case EK_PIN:
        sv.checkpoint = t->id;
        sv.pins |= (uint16_t)(1u << t->id);
        strokes = max_strokes();
        save_now();
        sfx_play_name("lnk_pin");
        /* a pin just shows its name */
        snprintf(banner, sizeof banner, "PIN: %s", LNK_PIN_NAME[t->id]);
        banner_t = 150;
        banner_col = C_YELLOW;
        burst(tcx(t), tcy(t) - 8, C_YELLOW, 10, 1.4f);
        break;
    case EK_IRON:
        if ((sv.irons >> t->id) & 1) break;
        sv.irons |= 1u << t->id;
        snprintf(buf, sizeof buf, "AN IRON! MOST STROKES: %d. (%d OF %d)", max_strokes(), popcount32(sv.irons), LNK_IRONS);
        got_item("IRON", buf);
        break;
    case EK_STRAY:
        if ((sv.strays >> t->id) & 1) break;
        sv.strays |= (uint8_t)(1u << t->id);
        snprintf(buf, sizeof buf, "%s (%d OF %d FOUND)", LNK_STRAY_LINE[t->id], popcount32(sv.strays), LNK_STRAYS);
        got_item(LNK_STRAY_NAME[t->id], buf);
        break;
    case EK_ABILITY:
        if (has_ab(t->id)) break;
        sv.abilities |= (uint8_t)(1u << t->id);
        sync_ctx();
        if (t->id == AB_HAMMER) game_award(GOAL_BEACON);
        got_item(LNK_ABILITY_NAME[t->id], LNK_ABILITY_LINE[t->id]);
        break;
    case EK_PIECE:
        if ((sv.pieces >> t->id) & 1) break;
        sv.pieces |= (uint8_t)(1u << t->id);
        snprintf(buf, sizeof buf, "A PIECE OF THE STAR PIN! (%d OF %d)", popcount32(sv.pieces), LNK_PIECES);
        got_item("STAR PIN", buf);
        break;
    case EK_NPC:
        demo_seen_npc[i] = true;
        sfx_play_name("lnk_talk");
        snprintf(buf, sizeof buf, "%s", LNK_NPC_LINE[t->id][(sv.runs + frame_t / 600) % 2 && LNK_NPC_LINE[t->id][1] ? 1 : 0]);
        talk(LNK_NPC_NAME[t->id], buf, TA_NONE);
        break;
    case EK_ALTAR:
        if (sv.placed) { talk("ALTAR", LNK_ALTAR_LINE[2], TA_NONE); break; }
        if (sv.pieces == 0xF) {
            sv.placed = 1;
            sync_ctx();
            save_now();
            shake_t = 40;
            music_play(LNK_MUS_ITEM);
            strokes = max_strokes();
            talk("ALTAR", LNK_ALTAR_LINE[2], TA_NONE);
        } else {
            snprintf(buf, sizeof buf, "%s (%d OF 4)", LNK_ALTAR_LINE[sv.pieces ? 1 : 0], popcount32(sv.pieces));
            talk("ALTAR", buf, TA_NONE);
        }
        break;
    case EK_PLATE:
        if ((plates_lit >> t->id) & 1) break;
        plates_lit |= (uint8_t)(1u << t->id);
        sfx_play_name("lnk_plate");
        burst(tcx(t), tcy(t), C_CYAN, 8, 1.2f);
        if (plates_lit == 0xF) {
            sync_ctx();
            shake_t = 40;
            talk(NULL, "FOUR PLATES GLOW. SOMEWHERE WEST, A DOOR SLIDES OPEN.", TA_NONE);
        }
        break;
    case EK_TREE:
        demo_seen_npc[i] = true;
        sv.secrets |= 1;
        save_now();
        talk(NULL, LNK_TREE_LINE, TA_NONE);
        break;
    case EK_SAUCER:
        demo_seen_npc[i] = true;
        sv.secrets |= 2;
        save_now();
        talk(NULL, LNK_SAUCER_LINE, TA_NONE);
        break;
    default: break;
    }
}

static void check_touches(void) {
    for (int i = 0; i < lnk_nthings; i++) {
        const LnkThing *t = thing_at(i);
        if (t->layer != ball.layer) continue;
        if (t->kind == EK_START || t->kind == EK_SLICER || t->kind == EK_LARK || t->kind == EK_ALBA || t->kind == EK_CROW ||
            t->kind == EK_DEN)
            continue;
        float dx = tcx(t) - ball.x, dy = tcy(t) - ball.y, d2 = dx * dx + dy * dy;
        float r = t->kind == EK_ALTAR ? 13.0f : t->kind == EK_SAUCER ? 18.0f : TOUCH_R;
        if (!touch_armed[i]) {
            if (d2 > (r + 10) * (r + 10)) touch_armed[i] = true;
            continue;
        }
        if (d2 < r * r) {
            touch_armed[i] = false;
            touch_thing(i);
            if (state != ST_PLAY) return;
        }
    }
}

/* ---- the creatures ---------------------------------------------------- */

static bool solid_px(int layer, float x, float y) {
    char ch = lnk_tile(&ctx, layer, (int)floorf(x / LNK_T), (int)floorf(y / LNK_T));
    return lnk_solid_char(ch) || ch == 'g' || ch == 'k';
}

static void knock_ball(float vx, float vy) {
    ball.sx = ball.x;
    ball.sy = ball.y;
    ball.slayer = ball.layer;
    ball.slie = ball.lie == LIE_CUP ? LIE_GROUND : ball.lie;
    ball.vx = vx;
    ball.vy = vy;
    ball.z = ball.vz = 0;
    ball.lie = LIE_GROUND;
    ball.moving = true;
    ball.slow_t = ball.roll_t = 0;
    ball.hopped = true;
}

static void update_movers(void) {
    float bsp = sqrtf(ball.vx * ball.vx + ball.vy * ball.vy);
    for (int i = 0; i < n_movers; i++) {
        Mover *m = &movers[i];
        m->t++;
        bool same = m->layer == ball.layer;
        float dx = ball.x - m->x, dy = ball.y - m->y, d = sqrtf(dx * dx + dy * dy);
        switch (m->kind) {
        case EK_SLICER:
            if (m->state == SL_GONE) break;
            if (m->state == SL_KNOCKED) {
                /* tumbling: it can go in the water or down a hole */
                float nx = m->x + m->vx, ny = m->y + m->vy;
                if (solid_px(m->layer, nx, m->y)) m->vx = -m->vx * 0.5f; else m->x = nx;
                if (solid_px(m->layer, m->x, ny)) m->vy = -m->vy * 0.5f; else m->y = ny;
                m->vx *= 0.94f;
                m->vy *= 0.94f;
                char ch = lnk_tile(&ctx, m->layer, (int)floorf(m->x / LNK_T), (int)floorf(m->y / LNK_T));
                if (ch == '~' || ch == 'o' || ch == 'O') {
                    m->state = SL_GONE;
                    burst(m->x, m->y, ch == '~' ? C_SKY : C_GREY, 10, 1.3f);
                    sfx_play_name(ch == '~' ? "lnk_splash" : "lnk_cup");
                    gain_strokes(SLICER_STROKES);
                    break;
                }
                if (fabsf(m->vx) + fabsf(m->vy) < 0.1f) { m->state = SL_BACK; m->t = 0; }
                break;
            }
            if (same && ball.moving && bsp > 0.8f && d < 9 && ball.z < 4) {
                /* the ball knocks it flying */
                m->vx = ball.vx * 0.9f;
                m->vy = ball.vy * 0.9f;
                m->state = SL_KNOCKED;
                ball.vx *= 0.6f;
                ball.vy *= 0.6f;
                sfx_play_name("lnk_bonk");
                break;
            }
            if (m->state == SL_IDLE) {
                m->x = m->hx + sinf(m->t * 0.03f) * 10;
                m->y = m->hy + sinf(m->t * 0.05f) * 5;
                if (same && !ball.moving && d < 96 && state == ST_PLAY && !charging) { m->state = SL_CHASE; m->t = 0; }
            } else if (m->state == SL_CHASE) {
                if (!same || ball.moving || d > 140) { m->state = SL_BACK; m->t = 0; break; }
                float sp = 0.55f;
                float nx = m->x + dx / d * sp, ny = m->y + dy / d * sp;
                if (!solid_px(m->layer, nx, m->y)) m->x = nx;
                if (!solid_px(m->layer, m->x, ny)) m->y = ny;
                if (d < 7) {
                    /* the sting: a stroke gone, and a shove */
                    strokes = imax(0, strokes - 1);
                    sfx_play_name("lnk_sting");
                    burst(ball.x, ball.y, C_PINK, 6, 1.0f);
                    knock_ball(dx / d * 1.6f, dy / d * 1.6f);
                    m->state = SL_BACK;
                    m->t = 0;
                }
            } else if (m->state == SL_BACK) {
                float hx = m->hx - m->x, hy = m->hy - m->y, hd = sqrtf(hx * hx + hy * hy);
                if (hd < 2 || m->t > 400) { m->state = SL_IDLE; m->t = 0; m->x = m->hx; m->y = m->hy; }
                else if (m->t > 90) { m->x += hx / hd * 0.7f; m->y += hy / hd * 0.7f; }
            }
            break;
        case EK_LARK:
        case EK_ALBA:
            if (m->state) { m->y -= 1.2f; m->x += 0.8f; break; } /* flown off */
            if (same && d < (m->kind == EK_ALBA ? 12 : 9) && ball.z < 6) {
                m->state = 1;
                gain_strokes(m->kind == EK_ALBA ? ALBA_STROKES : LARK_STROKES);
                sfx_play_name("lnk_bird");
                burst(m->x, m->y, C_WHITE, 8, 1.0f);
            }
            break;
        case EK_CROW:
            if (m->state == CR_DEAD) break;
            if (m->state == CR_FLOWN) { if (m->t < 200) { m->y -= 1.5f; m->x += 1.0f; } break; }
            if (same && d < 10 && ball.z < 8) {
                /* smashed: strokes (more for a quick one) and its tape */
                int gain = 3 + (m->state == CR_WATCH ? crow_countdown[m->id] : CROW_WATCH);
                m->state = CR_DEAD;
                sv.crows |= (uint16_t)(1u << m->id);
                gain_strokes(gain);
                save_now();
                burst(m->x, m->y, C_GREY, 12, 1.4f);
                sfx_play_name("lnk_crash");
                music_play(LNK_MUS_TAPE);
                char buf[160];
                snprintf(buf, sizeof buf, "+%d STROKES. %s", gain, LNK_CROW_TAPE[m->id]);
                talk("SCORECROW", buf, TA_NONE);
                return;
            }
            if (m->state == CR_PERCH && same && d < CROW_RANGE) {
                m->state = CR_WATCH;
                crow_countdown[m->id] = CROW_WATCH;
                sfx_play_name("lnk_caw");
            }
            break;
        }
    }
}

/* ---- the Brass Badger --------------------------------------------------- */

static float burrow_x(int s) { return BURROW[s][0] * LNK_T + LNK_T / 2.0f; }
static float burrow_y(int s) { return BURROW[s][1] * LNK_T + LNK_T / 2.0f; }

static bool in_den(void) { return ball.layer == LNK_UNDER && ball.y < LNK_ZH * LNK_T && ball.x < LNK_ZW * LNK_T - 8; }

static void win_game(void);

static void update_badger(void) {
    if (bd.state == BD_DEAD) return;
    bd.t++;
    float bx = burrow_x(bd.spot), by = burrow_y(bd.spot);
    if (bd.state == BD_SLEEP) {
        if (in_den()) {
            bd.state = BD_UP;
            bd.spot = 0;
            bd.shots_left = BADGER_SHOTS;
            bd.t = 0;
            music_play(LNK_MUS_BADGER);
            talk("THE BRASS BADGER", "A HEAD OF DENTED BRASS POKES UP FROM THE FLOOR. IT COUNTS YOUR STROKES.", TA_NONE);
        }
        return;
    }
    /* the badger is solid: the ball bounces off it */
    float dx = ball.x - bx, dy = ball.y - by, d = sqrtf(dx * dx + dy * dy);
    if (ball.layer == LNK_UNDER && d < 13 && d > 0.01f && bd.state != BD_DIG) {
        float sp = sqrtf(ball.vx * ball.vx + ball.vy * ball.vy);
        if (bd.state == BD_UP && sp > 0.6f && ball.moving) {
            bd.hp--;
            shake_t = 16;
            sfx_play_name("lnk_clang");
            burst(bx, by - 6, C_AMBER, 12, 1.6f);
            if (bd.hp <= 0) {
                bd.state = BD_DEAD;
                bd.t = 0;
                ball.vx = ball.vy = 0;
                win_game();
                return;
            }
            bd.state = BD_STAND;
            bd.t = 0;
        }
        if (ball.moving) {
            ball.vx = dx / d * fmaxf(sp * 0.5f, 0.6f);
            ball.vy = dy / d * fmaxf(sp * 0.5f, 0.6f);
            ball.x = bx + dx / d * 13.5f;
            ball.y = by + dy / d * 13.5f;
        }
    }
    switch (bd.state) {
    case BD_UP: break;
    case BD_STAND:
        if (bd.t > 40) { bd.state = BD_FIRE; bd.t = 0; bd.volley = 0; bd.cross_on = false; }
        break;
    case BD_FIRE:
        if (!bd.cross_on) {
            bd.cross_on = true;
            bd.cx = ball.x;
            bd.cy = ball.y;
            bd.t = 0;
            sfx_play_name("lnk_aim");
        } else if (bd.t >= VOLLEY_T) {
            float ex = ball.x - bd.cx, ey = ball.y - bd.cy;
            burst(bd.cx, bd.cy, C_ORANGE, 12, 1.8f);
            sfx_play_name("lnk_boom");
            shake_t = 10;
            if (ex * ex + ey * ey < 14 * 14) {
                strokes = imax(0, strokes - VOLLEY_COST);
                float el = sqrtf(ex * ex + ey * ey);
                if (el < 0.5f) { ex = 1; el = 1; }
                knock_ball(ex / el * 2.0f, ey / el * 2.0f);
            }
            bd.cross_on = false;
            if (++bd.volley >= VOLLEYS) { bd.state = BD_DIG; bd.t = 0; }
        }
        break;
    case BD_DIG:
        if (bd.t > 50) {
            int s;
            do s = rng_range(&g_rng, 0, 4); while (s == bd.spot);
            bd.spot = s;
            bd.state = BD_UP;
            bd.shots_left = BADGER_SHOTS;
            bd.t = 0;
            sfx_play_name("lnk_dig");
        }
        break;
    }
    if (strokes <= 0 && !ball.moving) check_out();
}

/* ---- play --------------------------------------------------------------- */

static int zone_of_ball(void) {
    int zx = iclamp((int)(ball.x / (LNK_ZW * LNK_T)), 0, LNK_ZX - 1), zy = iclamp((int)(ball.y / (LNK_ZH * LNK_T)), 0, LNK_ZY - 1);
    return ball.layer * 100 + zy * 10 + zx;
}

static int zone_music(void) {
    if (ball.layer == LNK_UNDER) {
        if (in_den() && bd.state != BD_SLEEP && bd.state != BD_DEAD) return LNK_MUS_BADGER;
        if (in_finale()) return LNK_MUS_RUINS;
        return LNK_MUS_CAVES;
    }
    int zx = (int)(ball.x / (LNK_ZW * LNK_T)), zy = (int)(ball.y / (LNK_ZH * LNK_T));
    if (zy <= 1 && zx <= 1) return LNK_MUS_RUINS;
    if (zx >= 6 && zy <= 2) return LNK_MUS_FEN;
    if (zx <= 1 && zy >= 4) return LNK_MUS_FEN;
    if ((zx >= 5 && zy >= 4) || (zx >= 6 && zy == 3)) return LNK_MUS_SANDS;
    return LNK_MUS_LINKS;
}

/* safe zone: a hole with a pin close by tops the strokes up */
static void safe_zone(int tx, int ty) {
    for (int i = 0; i < lnk_nthings; i++) {
        const LnkThing *t = thing_at(i);
        if (t->kind != EK_PIN) continue;
        if (iabs(t->tx - tx) <= 5 && iabs(t->ty - ty) <= 5) {
            if (strokes < SAFE_TOP) strokes = imin(SAFE_TOP, max_strokes());
            return;
        }
    }
}

/* the d-pad points the aim: it swings towards the pad's way, one notch a tap */
static int pad_angle(void) {
    int x = btn(BTN_RIGHT) - btn(BTN_LEFT), y = btn(BTN_DOWN) - btn(BTN_UP);
    if (!x && !y) return -1;
    static const int A[3][3] = {{40, 48, 56}, {32, -1, 0}, {24, 16, 8}}; /* [y+1][x+1] */
    return A[y + 1][x + 1];
}

static void aim_update(void) {
    int a = pad_angle();
    if (a < 0) { aim_hold = 0; return; }
    int diff = ((a - aim) % LNK_DIRS + LNK_DIRS + LNK_DIRS / 2) % LNK_DIRS - LNK_DIRS / 2;
    if (diff == 0) { aim_hold++; return; }
    int every = aim_hold > 16 ? 1 : 4;
    if (aim_hold == 0 || aim_hold % every == 0) {
        aim = (aim + isign(diff) + LNK_DIRS) % LNK_DIRS;
        if (aim_hold == 0) sfx_play_name("lnk_tick");
    }
    aim_hold++;
}

static int meter_level(void) { return iclamp(1 + (int)(meter_p * LNK_LEVELS), 1, LNK_LEVELS); }

static void play_update(void) {
    frame_t++;
    sync_ctx();
    int z = zone_of_ball();
    if (z != last_zone) {
        last_zone = z;
        int zx = z % 10, zy = (z / 10) % 10;
        snprintf(banner, sizeof banner, "%s", LNK_ZONE_NAME[z / 100][zy][zx]);
        banner_t = 150;
        banner_col = C_CREAM;
    }
    if (banner_t > 0) banner_t--;
    music_play(zone_music());

    if (!ball.moving) {
        if (charging) {
            if (btnp(BTN_B)) { charging = false; sfx_play_name("lnk_cancel"); }
            else if (!btn(BTN_A)) shoot();
            else {
                /* the power swings up and down, slower the longer you hold */
                meter_hold++;
                float rate = 1.0f / fminf(40.0f + meter_hold / 4.0f, 100.0f);
                meter_p += rate * meter_dir;
                if (meter_p >= 1.0f) { meter_p = 1.0f - (meter_p - 1.0f); meter_dir = -1; }
                if (meter_p <= 0.0f) { meter_p = -meter_p; meter_dir = 1; }
                level_shown = meter_level();
            }
        } else {
            if (btn(BTN_B)) {
                b_held++;
                if (b_held > 8) {
                    panning = true;
                    pan_x = iclamp(pan_x + (btn(BTN_RIGHT) - btn(BTN_LEFT)) * 3, -200, 200);
                    pan_y = iclamp(pan_y + (btn(BTN_DOWN) - btn(BTN_UP)) * 3, -130, 130);
                }
            } else {
                b_held = 0;
                panning = false;
                pan_x = pan_y = 0;
                aim_update();
                if (btnp(BTN_A) && (strokes > 0 || ball.lie == LIE_CUP)) {
                    charging = true;
                    meter_p = 0;
                    meter_dir = 1;
                    meter_hold = 0;
                    level_shown = 1;
                    sfx_play_name("lnk_charge");
                }
            }
        }
    } else {
        charging = false;
        panning = false;
        pan_x = pan_y = 0;
        /* A again: the Dune Tread's hop. B: Backspin's brake */
        if (btnp(BTN_A) && lnk_hop(&ball, &ctx)) sfx_play_name("lnk_hop");
        if (btnp(BTN_B) && lnk_brake(&ball, &ctx)) sfx_play_name("lnk_brake");
        /* a ball that can't settle can be popped along with the pad */
        float sp = sqrtf(ball.vx * ball.vx + ball.vy * ball.vy);
        if (sp < 0.5f) unsteady_t++; else unsteady_t = 0;
        if (pop_cool > 0) pop_cool--;
        if (unsteady_t > 90 && pop_cool == 0) {
            int a = pad_angle();
            if (a >= 0) { ball.vx += lnk_dir_x(a); ball.vy += lnk_dir_y(a); pop_cool = 30; sfx_play_name("lnk_tick"); }
        }
        int ev = lnk_ball_step(&ball, &ctx);
        switch (ev) {
        case EV_BOUNCE: sfx_play_name("lnk_bounce"); break;
        case EV_LAND: sfx_play_name("lnk_land"); break;
        case EV_BLOCK:
            if (LNK_MAP[ball.layer][ball.by][ball.bx] == 'b') {
                sfx_play_name("lnk_bush");
                burst(ball.bx * LNK_T + 8, ball.by * LNK_T + 8, C_JADE, 10, 1.2f);
            } else {
                sfx_play_name("lnk_smash");
                shake_t = 8;
                burst(ball.bx * LNK_T + 8, ball.by * LNK_T + 8, C_ORANGE, 12, 1.5f);
            }
            break;
        case EV_SINK:
            sfx_play_name("lnk_splash");
            burst(ball.x, ball.y, C_SKY, 8, 1.0f);
            break;
        case EV_HOLE:
            sfx_play_name("lnk_cup");
            state = ST_WARP;
            warp_t = 0;
            return;
        default: break;
        }
        check_touches();
        if (state != ST_PLAY) return;
    }
    update_movers();
    if (state != ST_PLAY) return;
    update_badger();
    if (state != ST_PLAY) return;
    stray_help();
    if (state != ST_PLAY) return;
    check_out();
}

static void warp_update(void) {
    warp_t++;
    if (warp_t == 16) {
        ball.layer ^= 1;
        ball.lie = LIE_CUP;
        ball.moving = false;
        cam_x = ball.x - SCREEN_W / 2;
        cam_y = ball.y - SCREEN_H / 2;
        safe_zone(ball.hx, ball.hy);
        for (int i = 0; i < lnk_nthings; i++) {
            const LnkThing *t = thing_at(i);
            float dx = tcx(t) - ball.x, dy = tcy(t) - ball.y;
            if (t->layer == ball.layer && dx * dx + dy * dy < TOUCH_R * TOUCH_R) touch_armed[i] = false;
        }
    }
    if (warp_t >= 32) { state = ST_PLAY; state_t = 0; }
}

static void win_game(void) {
    sv.won = 1;
    sv.wins++;
    if (!sv.best_strokes || stroke_run < sv.best_strokes) sv.best_strokes = (uint16_t)imin(stroke_run, 65535);
    game_award(GOAL_SAUCER);
    if (all_done()) {
        sv.won_all = 1;
        game_award(GOAL_ALIEN);
    }
    save_now();
    state = ST_END;
    state_t = 0;
    end_i = 0;
    music_play(LNK_MUS_END);
}

/* ------------------------------------------------------------------ */
/* the cartridge's own screens                                          */

static void title_update(void) {
    state_t++;
    if (title_confirm) {
        if (btnp(BTN_LEFT) || btnp(BTN_RIGHT)) { title_confirm = 3 - title_confirm; sfx_play_name("ui_move"); }
        if (btnp(BTN_B)) { title_confirm = 0; sfx_play_name("ui_back"); }
        else if (btnp(BTN_A)) {
            if (title_confirm == 2) { sfx_play_name("ui_ok"); new_game(); }
            else sfx_play_name("ui_back");
            title_confirm = 0;
        }
        return;
    }
    int n = has_save ? 2 : 1;
    if (btnp(BTN_UP) || btnp(BTN_DOWN)) { title_sel = (title_sel + 1) % n; sfx_play_name("ui_move"); }
    if (btnp(BTN_B)) { game_exit_to_library(); return; }
    if (btnp(BTN_A) || btnp(BTN_START)) {
        sfx_play_name("ui_ok");
        if (has_save && title_sel == 0) {
            game_set_pausable(true);
            stroke_run = 0;
            start_run();
        } else if (has_save) {
            title_confirm = 1;
        } else {
            game_set_pausable(true);
            new_game();
        }
    }
}

static void lnk_update(void) {
    if (shake_t > 0) shake_t--;
    for (int i = 0; i < ARRAY_LEN(parts); i++)
        if (parts[i].life > 0) { parts[i].x += parts[i].vx; parts[i].y += parts[i].vy; parts[i].vy += 0.04f; parts[i].life--; }
    switch (state) {
    case ST_TITLE: title_update(); break;
    case ST_INTRO:
        state_t++;
        if (state_t > 20 && btnp(BTN_A)) {
            state_t = 0;
            if (!LNK_INTRO[++intro_i]) { state = ST_PLAY; }
        }
        break;
    case ST_PLAY: play_update(); break;
    case ST_TALK:
        state_t++;
        if (state_t > 12 && (btnp(BTN_A) || btnp(BTN_B))) {
            state = ST_PLAY;
            state_t = 0;
            if (talk_after == TA_END) state = ST_END;
        }
        break;
    case ST_WARP: warp_update(); break;
    case ST_OUT:
        state_t++;
        if (state_t > 60 && (btnp(BTN_A) || state_t > 400)) start_run();
        break;
    case ST_END:
        state_t++;
        if (state_t > 40 && btnp(BTN_A)) {
            state_t = 0;
            end_i++;
            int n1 = 0, n2 = 0;
            while (LNK_ENDING[n1]) n1++;
            while (LNK_ENDING_ALL[n2]) n2++;
            int total = n1 + (sv.won_all ? n2 : 0) + 1; /* then the tally */
            if (end_i >= total) {
                /* back to the title: the links stay yours to roll round */
                state = ST_TITLE;
                title_sel = 0;
                game_set_pausable(false);
                music_play(LNK_MUS_TITLE);
            }
        }
        break;
    }
    if (state == ST_PLAY || state == ST_TALK || state == ST_WARP) {
        float tx = ball.x - SCREEN_W / 2 + pan_x, ty = ball.y - SCREEN_H / 2 + pan_y;
        cam_x += (tx - cam_x) * 0.18f;
        cam_y += (ty - cam_y) * 0.18f;
        cam_x = fclamp(cam_x, 0, LNK_MW * LNK_T - SCREEN_W);
        cam_y = fclamp(cam_y, -14, LNK_MH * LNK_T - SCREEN_H);
    }
}

/* ------------------------------------------------------------------ */
/* drawing                                                              */

static void draw_world(void) {
    int cx = (int)cam_x + (shake_t > 0 ? ((shake_t / 2) % 2 ? 2 : -2) : 0), cy = (int)cam_y;
    int l = ball.layer;
    gfx_cls(l == LNK_UNDER ? C_INK : C_FOREST);
    int tx0 = cx / LNK_T - (cx < 0), ty0 = cy / LNK_T - (cy < 0);
    for (int ty = ty0; ty <= ty0 + SCREEN_H / LNK_T + 1; ty++)
        for (int tx = tx0; tx <= tx0 + SCREEN_W / LNK_T + 1; tx++) {
            char ch = (tx < 0 || ty < 0 || tx >= LNK_MW || ty >= LNK_MH) ? '#' : (char)tiles[l][ty][tx];
            if ((ch == 'g' && (ctx.opened & 1)) || (ch == 'k' && (ctx.opened & 2))) ch = '.';
            lnk_draw_tile(l, ch, tx, ty, tx * LNK_T - cx, ty * LNK_T - cy, frame_t, panning, tiles);
        }
    /* things */
    for (int i = 0; i < lnk_nthings; i++) {
        const LnkThing *t = thing_at(i);
        if (t->layer != l) continue;
        int x = t->tx * LNK_T - cx, y = t->ty * LNK_T - cy;
        if (x < -40 || y < -40 || x > SCREEN_W + 40 || y > SCREEN_H + 40) continue;
        int bob = (frame_t / 12 + i) % 2;
        switch (t->kind) {
        case EK_PIN: spr_draw(&lnk_spr[(sv.pins >> t->id) & 1 ? LS_PIN_LIT : LS_PIN], x + 4, y - 10, 0); break;
        case EK_IRON: if (!((sv.irons >> t->id) & 1)) spr_draw(&lnk_spr[LS_IRON], x + 2, y - bob, 0); break;
        case EK_STRAY: if (!((sv.strays >> t->id) & 1)) spr_draw(&lnk_spr[LS_STRAY], x + 4, y + 4 - bob, 0); break;
        case EK_NPC:
            spr_draw(&lnk_spr[t->id == 9 ? LS_SAGE : (t->layer == LNK_UNDER || t->id == 1 || t->id == 4) ? LS_FOLK : LS_STRAY], x + 3, y + 1, 0);
            break;
        case EK_ABILITY:
            if (!has_ab(t->id)) spr_draw(&lnk_spr[LS_AB_HAMMER + t->id], x + 2, y + 2 - bob, 0);
            break;
        case EK_PIECE: if (!((sv.pieces >> t->id) & 1)) spr_draw(&lnk_spr[LS_PIECE], x + 2, y + 2 - bob, 0); break;
        case EK_ALTAR: spr_draw(&lnk_spr[LS_ALTAR], x, y - 4, 0); if (sv.placed) spr_draw(&lnk_spr[LS_PIECE], x + 2, y - 8, 0); break;
        case EK_PLATE: spr_draw(&lnk_spr[(plates_lit >> t->id) & 1 ? LS_PLATE_LIT : LS_PLATE], x, y, 0); break;
        case EK_TREE: spr_draw(&lnk_spr[LS_TREE_ODD], x - 2, y - 6, 0); break;
        case EK_SAUCER: spr_draw(&lnk_spr[LS_SAUCER], x - 8, y - 2, 0); spr_draw(&lnk_spr[LS_STRAY], x + 26, y + 6, 0); break;
        case EK_DEN: break;
        default: break;
        }
    }
    /* creatures */
    for (int i = 0; i < n_movers; i++) {
        Mover *m = &movers[i];
        if (m->layer != l) continue;
        int x = (int)m->x - cx, y = (int)m->y - cy;
        if (x < -20 || y < -20 || x > SCREEN_W + 20 || y > SCREEN_H + 20) continue;
        switch (m->kind) {
        case EK_SLICER:
            if (m->state == SL_GONE) break;
            spr_draw(&lnk_spr[(frame_t / 3) % 2 ? LS_SLICER1 : LS_SLICER2], x - 4, y - 8, m->state == SL_KNOCKED ? SPR_FLIPY : 0);
            gfx_rect(x - 2, y + 3, 4, 1, C_INK);
            break;
        case EK_LARK:
            if (m->state == 0) spr_draw(&lnk_spr[(frame_t / 20 + i) % 6 ? LS_LARK1 : LS_LARK2], x - 5, y - 4, 0);
            else if (y > -10) spr_draw(&lnk_spr[(frame_t / 4) % 2 ? LS_LARK1 : LS_LARK2], x - 5, y - 4, 0);
            break;
        case EK_ALBA:
            spr_draw(&lnk_spr[(frame_t / (m->state ? 5 : 30)) % 2 ? LS_ALBA1 : LS_ALBA2], x - 8, y - 8, 0);
            break;
        case EK_CROW:
            if (m->state == CR_DEAD) { spr_draw(&lnk_spr[LS_CROW_DEAD], x - 6, y - 4, 0); break; }
            spr_draw(&lnk_spr[m->state == CR_FLOWN || (m->state == CR_WATCH && (frame_t / 8) % 2) ? LS_CROW2 : LS_CROW1], x - 6, y - 8, 0);
            if (m->state == CR_WATCH) {
                char b[4];
                snprintf(b, sizeof b, "%d", crow_countdown[m->id]);
                gfx_rect(x - 3, y - 17, 7, 8, C_RED);
                tiny_draw(b, x - 1, y - 15, C_WHITE);
            }
            break;
        }
    }
    /* the badger */
    if (l == LNK_UNDER && bd.state != BD_SLEEP) {
        int x = (int)burrow_x(bd.spot) - cx, y = (int)burrow_y(bd.spot) - cy;
        if (bd.state == BD_DEAD) {
            if (bd.t < 60) spr_draw(&lnk_spr[LS_BADGER_DOWN], x - 16, y - 14, 0);
        } else if (bd.state == BD_DIG) {
            gfx_dither_circle(x, y + 4, 10, C_BROWN, 8 + (frame_t % 4));
        } else {
            gfx_circ(x, y + 6, 11, C_INK);
            spr_draw(&lnk_spr[bd.state == BD_UP ? LS_BADGER_HEAD : LS_BADGER_STAND], x - 16, y - (bd.state == BD_UP ? 14 : 26), 0);
            if (bd.state == BD_UP) {
                char b[4];
                snprintf(b, sizeof b, "%d", bd.shots_left);
                tiny_draw(b, x - 1, y - 24, C_YELLOW);
            }
        }
        if (bd.cross_on) {
            int r = 4 + (VOLLEY_T - bd.t) / 6;
            int ccx = (int)bd.cx - cx, ccy = (int)bd.cy - cy;
            gfx_circb(ccx, ccy, r, (frame_t / 4) % 2 ? C_RED : C_ORANGE);
            spr_draw(&lnk_spr[LS_CROSSHAIR], ccx - 7, ccy - 7, 0);
        }
    }
    /* the aiming line: the first stretch of the roll, bending with the ground */
    if (!ball.moving && (state == ST_PLAY) && !panning) {
        LnkBall g = ball;
        LnkCtx pc = ctx;
        pc.probe = true;
        lnk_hit(&g, &pc, aim, charging ? level_shown : 5);
        int steps = lnk_is_chip(&ball, &ctx) ? 30 : 36;
        for (int f = 0; f < steps; f++) {
            int ev = lnk_ball_step(&g, &pc);
            if (f % 3 == 2) {
                int px = (int)g.x - cx, py = (int)(g.y - g.z) - cy;
                gfx_rect(px, py, 2, 2, f / 3 % 2 ? C_WHITE : C_LIGHT);
            }
            if (ev == EV_HOLE || ev == EV_SINK || ev == EV_REST) break;
        }
    }
    /* the ball */
    int bx = (int)ball.x - cx, by = (int)ball.y - cy;
    if (state != ST_WARP || warp_t < 8 || warp_t > 24) {
        if (ball.z > 0) spr_draw(&lnk_spr[LS_BALL_SHADOW], bx - 3, by - 1, 0);
        spr_draw(&lnk_spr[LS_BALL], bx - 3, by - 3 - (int)ball.z, 0);
    }
    for (int i = 0; i < ARRAY_LEN(parts); i++)
        if (parts[i].life > 0) gfx_rect((int)parts[i].x - cx, (int)parts[i].y - cy, 2, 2, parts[i].col);
    if (panning) {
        gfx_rectb(2, 14, SCREEN_W - 4, SCREEN_H - 16, C_YELLOW);
        tiny_draw("LOOKING", 6, SCREEN_H - 9, C_YELLOW);
    }
}

static void draw_hud(void) {
    gfx_rect(0, 0, SCREEN_W, 12, C_INK);
    char b[64];
    int mx = max_strokes();
    spr_draw(&lnk_spr[LS_BALL], 4, 3, 0);
    snprintf(b, sizeof b, "%d/%d", strokes, mx);
    text_draw(b, 13, 2, strokes <= 3 ? ((frame_t / 10) % 2 ? C_RED : C_ORANGE) : C_WHITE);
    /* collected, in small */
    int x = 64;
    for (int a = 0; a < AB_COUNT; a++) {
        gfx_rect(x + a * 7, 3, 5, 5, has_ab(a) ? (a == 0 ? C_ORANGE : a == 1 ? C_PINK : a == 2 ? C_TAN : C_SKY) : C_NIGHT);
    }
    x += 32;
    for (int p = 0; p < LNK_PIECES; p++) gfx_rect(x + p * 4, 3, 3, 5, (sv.pieces >> p) & 1 ? C_YELLOW : C_NIGHT);
    int zx = iclamp((int)(ball.x / (LNK_ZW * LNK_T)), 0, LNK_ZX - 1), zy = iclamp((int)(ball.y / (LNK_ZH * LNK_T)), 0, LNK_ZY - 1);
    const char *zn = LNK_ZONE_NAME[ball.layer][zy][zx];
    if (banner_t > 0) {
        int w = text_width(banner);
        gfx_rect(SCREEN_W / 2 - w / 2 - 4, 20, w + 8, 11, C_INK);
        text_draw(banner, SCREEN_W / 2 - w / 2, 22, banner_col);
    }
    tiny_draw(zn, SCREEN_W - 4 - tiny_width(zn), 4, C_SLATE);
    if (charging) {
        /* the power meter: twelve pips */
        int y = SCREEN_H - 12;
        gfx_rect(SCREEN_W / 2 - 50, y - 2, 100, 9, C_INK);
        for (int i = 0; i < LNK_LEVELS; i++) {
            int col = i < level_shown ? (i < 4 ? C_LIME : i < 8 ? C_YELLOW : C_RED) : C_NIGHT;
            gfx_rect(SCREEN_W / 2 - 48 + i * 8, y, 6, 5, col);
        }
    }
    if (ball.lie == LIE_CUP && !ball.moving && state == ST_PLAY) tiny_draw("FREE CHIP OUT OF THE CUP", 6, SCREEN_H - 9, C_LIME);
    else if (lnk_is_chip(&ball, &ctx) && !ball.moving && state == ST_PLAY) tiny_draw("CHIP", 6, SCREEN_H - 9, C_TAN);
}

static void draw_box(const char *title, const char *text) {
    int h = 46;
    int y = SCREEN_H - h - 4;
    ui_panel(8, y, SCREEN_W - 16, h, C_INK, C_CREAM);
    int ty = y + 5;
    if (title) { text_draw(title, 16, ty, C_YELLOW); ty += 10; }
    text_wrap(text, 16, ty, SCREEN_W - 32, C_WHITE, 9);
    if (state_t > 12 && (state_t / 16) % 2) text_draw(GLYPH_A, SCREEN_W - 22, y + h - 11, C_YELLOW);
}

static void draw_title(void) {
    gfx_cls(C_SKY);
    gfx_rect(0, 100, SCREEN_W, 80, C_LEAF);
    for (int i = 0; i < 6; i++) gfx_circ(20 + i * 60, 108, 30 + (i % 3) * 6, (i % 2) ? C_JADE : C_LEAF);
    gfx_rect(0, 118, SCREEN_W, 62, C_JADE);
    gfx_dither(0, 118, SCREEN_W, 62, C_FOREST, 4);
    /* the green, the pin and Dimple */
    gfx_circ(230, 140, 34, C_LIME);
    gfx_circ(238, 146, 5, C_INK);
    spr_draw(&lnk_spr[LS_PIN_LIT], 240, 116, 0);
    int bx = 70 + (int)(sinf(state_t * 0.02f) * 6);
    spr_draw_scaled(&lnk_spr[LS_DIMPLE_BIG], bx, 108, 2, 0);
    static const uint8_t grad[] = {C_WHITE, C_CREAM, C_LIME, C_LEAF};
    ui_fancy_center("LOST LINKS", SCREEN_W / 2, 18, 3, grad, 4, C_INK, C_FOREST);
    if (title_confirm) {
        ui_panel(70, 70, 180, 40, C_INK, C_RED);
        text_center("START OVER? THE SAVE GOES.", SCREEN_W / 2, 76, C_WHITE);
        text_draw("NO", 120, 92, title_confirm == 1 ? C_YELLOW : C_GREY);
        text_draw("YES", 180, 92, title_confirm == 2 ? C_YELLOW : C_GREY);
        return;
    }
    const char *items[2] = {has_save ? "CONTINUE" : "NEW GAME", "NEW GAME"};
    int n = has_save ? 2 : 1;
    for (int i = 0; i < n; i++) {
        int y = 70 + i * 12;
        text_center(items[i], SCREEN_W / 2, y, i == title_sel ? C_YELLOW : C_WHITE);
        if (i == title_sel) ui_cursor(SCREEN_W / 2 - text_width(items[i]) / 2 - 12, y, state_t);
    }
    if (has_save) {
        char b[96];
        snprintf(b, sizeof b, "IRONS %d/%d  STRAYS %d/%d  CROWS %d/%d  PIECES %d/4", popcount32(sv.irons), LNK_IRONS,
                 popcount32(sv.strays), LNK_STRAYS, popcount32(sv.crows), LNK_CROWS, popcount32(sv.pieces));
        gfx_rect(0, 164, SCREEN_W, 16, C_INK);
        tiny_center(b, SCREEN_W / 2, 167, C_LIGHT);
        if (sv.won) {
            snprintf(b, sizeof b, "BADGER BEATEN %d TIMES  BEST %d STROKES", sv.wins, sv.best_strokes);
            tiny_center(b, SCREEN_W / 2, 173, C_YELLOW);
        }
    }
}

static void draw_end(void) {
    gfx_cls(C_NIGHT);
    for (int i = 0; i < 40; i++) gfx_pset((i * 97 + state_t / 3) % SCREEN_W, (i * 53) % 90, C_GREY);
    gfx_rect(0, 120, SCREEN_W, 60, C_LEAF);
    spr_draw(&lnk_spr[LS_PIN_LIT], 150, 96, 0);
    spr_draw_scaled(&lnk_spr[LS_DIMPLE_BIG], 120, 100, 1, 0);
    int n1 = 0, n2 = 0;
    while (LNK_ENDING[n1]) n1++;
    while (LNK_ENDING_ALL[n2]) n2++;
    const char *line = NULL;
    char b[160];
    if (end_i < n1) line = LNK_ENDING[end_i];
    else if (sv.won_all && end_i < n1 + n2) line = LNK_ENDING_ALL[end_i - n1];
    else {
        snprintf(b, sizeof b, "THE END. STROKES THIS GAME: %d. IRONS %d/%d, STRAYS %d/%d, TAPES %d/%d.", stroke_run,
                 popcount32(sv.irons), LNK_IRONS, popcount32(sv.strays), LNK_STRAYS, popcount32(sv.crows), LNK_CROWS);
        line = b;
    }
    text_wrap(line, 20, 30, SCREEN_W - 40, C_WHITE, 10);
    if (state_t > 40 && (state_t / 16) % 2) text_draw(GLYPH_A, SCREEN_W - 20, 150, C_YELLOW);
}

/* the whole of one layer, two pixels a tile (test hook "mapview", for
 * checking the world and for the README's spoiler maps) */
static int map_view = -1;
static void draw_map_view(void) {
    gfx_cls(C_INK);
    int l = map_view;
    for (int y = 0; y < LNK_MH; y++)
        for (int x = 0; x < LNK_MW; x++) {
            char ch = LNK_MAP[l][y][x];
            int c;
            switch (ch) {
            case '#': c = l ? C_NIGHT : C_SLATE; break;
            case 'T': c = C_FOREST; break;
            case 'H': c = C_LIGHT; break;
            case '~': c = l ? C_NAVY : C_BLUE; break;
            case 's': c = l ? C_TAN : C_EARTH; break;
            case ',': c = l ? C_PURPLE : C_FOREST; break;
            case ':': c = l ? C_SLATE : C_LIME; break;
            case 'X': c = C_ORANGE; break;
            case 'b': c = C_FOREST; break;
            case 'u': case 'v': c = C_BROWN; break;
            case 'r': c = C_GREY; break;
            case 'o': case 'O': c = l ? C_YELLOW : C_INK; break;
            case 'g': case 'k': c = C_RED; break;
            default:
                if (ch >= '1' && ch <= '9') c = l ? C_SLATE : C_LEAF;
                else if (lnk_is_marker(ch)) c = ch == 'F' ? C_RED : ch == 'I' ? C_WHITE : ch == 'B' || ch == 'L' || ch == 'E' ? (l ? C_DUSK : C_JADE) : C_PINK;
                else c = l ? C_DUSK : C_JADE;
            }
            gfx_rect(x * 2, 16 + y * 2, 2, 2, c);
        }
    text_draw(l ? "UNDERGROUND" : "THE LINKS", 4, 4, C_WHITE);
}

static void lnk_draw(void) {
    if (map_view >= 0) { draw_map_view(); return; }
    switch (state) {
    case ST_TITLE: draw_title(); return;
    case ST_END: draw_end(); return;
    case ST_INTRO:
        gfx_cls(C_INK);
        spr_draw(&lnk_spr[LS_BALL], SCREEN_W / 2 - 3, 110, 0);
        if ((state_t / 20) % 2) gfx_pset(SCREEN_W / 2, 107, C_YELLOW);
        text_wrap(LNK_INTRO[intro_i], 40, 50, SCREEN_W - 80, C_CREAM, 10);
        return;
    default: break;
    }
    draw_world();
    draw_hud();
    if (state == ST_TALK) draw_box(talk_title, talk_buf);
    if (state == ST_WARP) gfx_set_fade(warp_t < 16 ? warp_t / 3 : (32 - warp_t) / 3);
    else gfx_set_fade(0);
    if (state == ST_OUT) {
        gfx_darken_rect(0, 0, SCREEN_W, SCREEN_H, 2);
        ui_panel(60, 60, 200, 50, C_INK, C_RED);
        text_center("OUT OF STROKES", SCREEN_W / 2, 68, C_RED);
        const char *where = sv.checkpoint == 0xFF ? "WAKING HOLLOW" : LNK_PIN_NAME[sv.checkpoint];
        char b[64];
        snprintf(b, sizeof b, "BACK TO %s", where);
        text_center(b, SCREEN_W / 2, 84, C_WHITE);
        tiny_center("EVERYTHING YOU FOUND IS KEPT", SCREEN_W / 2, 98, C_GREY);
    }
}

/* ------------------------------------------------------------------ */
/* cartridge interface                                                  */

static void lnk_load(void) {
    lnk_world_scan();
    lnk_art_load();
    lnk_audio_load();
}

static void lnk_start(void) {
    lnk_world_scan();
    has_save = load_save();
    if (!has_save) { memset(&sv, 0, sizeof sv); sv.checkpoint = 0xFF; }
    lnk_tiles_reset(tiles);
    sync_ctx();
    state = ST_TITLE;
    state_t = 0;
    title_sel = 0;
    title_confirm = 0;
    frame_t = 0;
    banner_t = 0;
    memset(parts, 0, sizeof parts);
    bot_wp = 0;
    bot_flow_wp = -1;
    bot_have = false;
    game_set_pausable(false);
    music_play(LNK_MUS_TITLE);
}

static void lnk_quit(void) {
    if (has_save) save_now();
}

static void lnk_label(int x, int y, int w, int h, int t) {
    gfx_rect(x, y, w, h, C_SKY);
    gfx_rect(x, y + 30, w, h - 30, C_LEAF);
    gfx_dither(x, y + 40, w, h - 40, C_JADE, 6);
    gfx_circ(x + 100, y + 44, 18, C_LIME);
    gfx_circ(x + 106, y + 46, 3, C_INK);
    spr_draw(&lnk_spr[LS_PIN_LIT], x + 106, y + 24, 0);
    int bx = x + 20 + (t % 180) / 2;
    if (bx > x + 103) bx = x + 103;
    spr_draw(&lnk_spr[LS_BALL], bx, y + 42 + (bx >= x + 103 ? 3 : 0), 0);
    spr_draw(&lnk_spr[(t / 3) % 2 ? LS_SLICER1 : LS_SLICER2], x + 30 + (int)(sinf(t * 0.05f) * 8), y + 14, 0);
    static const uint8_t grad[] = {C_WHITE, C_LIME};
    ui_fancy_text("LOST LINKS", x + 4, y + 3, 1, grad, 2, C_INK, -1);
}

/* ---- the demo player ---------------------------------------------------- */

/* The order the demo player goes round the world in: every thing that
 * counts, and the pins it needs on the way. */
typedef struct Wp { uint8_t kind, id; } Wp;
static const Wp BOT_ROUTE[] = {
#include "lostlinks_route.h"
};

static bool wp_done(const Wp *w) {
    switch (w->kind) {
    case EK_PIN: return sv.checkpoint == w->id;
    case EK_IRON: return (sv.irons >> w->id) & 1;
    case EK_CROW: return (sv.crows >> w->id) & 1;
    case EK_STRAY: return (sv.strays >> w->id) & 1;
    case EK_ABILITY: return has_ab(w->id);
    case EK_PIECE: return (sv.pieces >> w->id) & 1;
    case EK_ALTAR: return sv.placed;
    case EK_PLATE: return (plates_lit >> w->id) & 1;
    case EK_DEN: return sv.won && state != ST_PLAY;
    default: {
        for (int i = 0; i < lnk_nthings; i++)
            if (lnk_things[i].kind == w->kind && lnk_things[i].id == w->id) return demo_seen_npc[i];
        return true;
    }
    }
}

static int dir_buttons(int a) {
    static const int B[8] = {BTN_RIGHT, BTN_RIGHT | BTN_DOWN, BTN_DOWN, BTN_DOWN | BTN_LEFT,
                             BTN_LEFT, BTN_LEFT | BTN_UP, BTN_UP, BTN_UP | BTN_RIGHT};
    return B[((a + LNK_DIRS) % LNK_DIRS) / 8];
}

static int bot_buttons(void) {
    switch (state) {
    case ST_TITLE: return (engine_frame() % 2) ? BTN_A : 0;
    case ST_INTRO: case ST_TALK: case ST_OUT: case ST_END: return (engine_frame() % 2) ? BTN_A : 0;
    case ST_WARP: return 0;
    default: break;
    }
    int n = ARRAY_LEN(BOT_ROUTE);
    while (bot_wp < n && wp_done(&BOT_ROUTE[bot_wp])) { bot_wp++; bot_have = false; }
    if (bot_wp >= n) return 0;
    const Wp *w = &BOT_ROUTE[bot_wp];
    const LnkThing *t = lnk_find(w->kind, w->id);
    if (!t) { bot_wp++; return 0; }
    float px = tcx(t), py = tcy(t);
    int tl = t->layer;
    if (w->kind == EK_DEN) {
        /* in the den: the badger's head, or away from the crosshair */
        if (bd.state == BD_UP) { px = burrow_x(bd.spot); py = burrow_y(bd.spot); }
        else if (bd.cross_on) {
            int far = 0;
            float fd = -1;
            for (int s = 0; s < 5; s++) {
                float dx = burrow_x(s) - bd.cx, dy = burrow_y(s) - bd.cy;
                if (s != bd.spot && dx * dx + dy * dy > fd) { fd = dx * dx + dy * dy; far = s; }
            }
            px = burrow_x(far) + 20;
            py = burrow_y(far);
        } else if (bd.state != BD_SLEEP) return 0; /* wait for it to pop up */
    }
    if (ball.moving) {
        if (bot_hop_at >= 0 && ball.roll_t == bot_hop_at) return BTN_A;
        if (bot_brake_at >= 0 && ball.roll_t == bot_brake_at) return BTN_B;
        return 0;
    }
    if (charging) {
        if (level_shown == bot_shot.level && meter_hold > 2) return 0; /* let go */
        return BTN_A;
    }
    if (!bot_have) {
        if (!bot_flow_ok || bot_flow_wp != bot_wp || bot_flow_ab != sv.abilities || bot_flow_open != ctx.opened) {
            lnk_flow_to(&bot_flow, &ctx, tl, t->tx, t->ty);
            bot_flow_ok = true;
            bot_flow_wp = bot_wp;
            bot_flow_ab = sv.abilities;
            bot_flow_open = ctx.opened;
        }
        if (!lnk_plan_shot(&ball, &ctx, &bot_flow, tl, px, py, w->kind == EK_DEN ? 13.0f : 8.0f, &bot_shot)) {
            bot_fail++;
            bot_shot.dir = (int)(rng_next(&g_rng) % LNK_DIRS);
            bot_shot.level = 6;
            bot_shot.hop_at = bot_shot.brake_at = -1;
        }
        bot_have = true;
        bot_shots++;
    }
    if (aim != bot_shot.dir) {
        int diff = ((bot_shot.dir - aim) % LNK_DIRS + LNK_DIRS + LNK_DIRS / 2) % LNK_DIRS - LNK_DIRS / 2;
        int s = isign(diff);
        /* a pad way at or just past the aim wanted, turning the short way */
        int a = bot_shot.dir;
        if (iabs(diff) >= 24) a = ((aim + s * 16) / 8) * 8; /* far: swing a quarter turn at a time */
        else if (a % 8) a = s > 0 ? (a / 8 + 1) * 8 : (a / 8) * 8;
        return dir_buttons((a + LNK_DIRS) % LNK_DIRS);
    }
    bot_hop_at = bot_shot.hop_at;
    bot_brake_at = bot_shot.brake_at;
    bot_have = false; /* plan afresh after this stroke */
    return BTN_A;
}

/* ---- test hooks --------------------------------------------------------- */

static int count_reach(const LnkFlow *f, int kind) {
    int n = 0;
    for (int i = 0; i < lnk_nthings; i++) {
        const LnkThing *t = thing_at(i);
        if (t->kind == kind && f->d[t->layer][t->ty][t->tx] != LNK_FAR) n++;
    }
    return n;
}

static int lnk_query(const char *key, int *out) {
    static LnkFlow reach;
    if (!strcmp(key, "state")) { *out = state; return 1; }
    if (!strcmp(key, "bot")) { *out = bot_buttons(); return 1; }
    if (!strcmp(key, "bot_wp")) { *out = bot_wp; return 1; }
    if (!strcmp(key, "bot_wps")) { *out = ARRAY_LEN(BOT_ROUTE); return 1; }
    if (!strcmp(key, "bot_shots")) { *out = bot_shots; return 1; }
    if (!strcmp(key, "bot_dir")) { *out = bot_shot.dir; return 1; }
    if (!strcmp(key, "bot_level")) { *out = bot_shot.level; return 1; }
    if (!strcmp(key, "bot_have")) { *out = bot_have; return 1; }
    if (!strcmp(key, "strokes")) { *out = strokes; return 1; }
    if (!strcmp(key, "max_strokes")) { *out = max_strokes(); return 1; }
    if (!strcmp(key, "aim")) { *out = aim; return 1; }
    if (!strcmp(key, "level")) { *out = level_shown; return 1; }
    if (!strcmp(key, "charging")) { *out = charging; return 1; }
    if (!strcmp(key, "panning")) { *out = panning; return 1; }
    if (!strcmp(key, "pan_x")) { *out = pan_x; return 1; }
    if (!strcmp(key, "moving")) { *out = ball.moving; return 1; }
    if (!strcmp(key, "airborne")) { *out = ball.z > 0; return 1; }
    if (!strcmp(key, "lie")) { *out = ball.lie; return 1; }
    if (!strcmp(key, "layer")) { *out = ball.layer; return 1; }
    if (!strcmp(key, "bx")) { *out = (int)ball.x; return 1; }
    if (!strcmp(key, "by")) { *out = (int)ball.y; return 1; }
    if (!strcmp(key, "btx")) { *out = (int)(ball.x / LNK_T); return 1; }
    if (!strcmp(key, "bty")) { *out = (int)(ball.y / LNK_T); return 1; }
    if (!strcmp(key, "speed100")) { *out = (int)(sqrtf(ball.vx * ball.vx + ball.vy * ball.vy) * 100); return 1; }
    if (!strcmp(key, "irons")) { *out = popcount32(sv.irons); return 1; }
    if (!strcmp(key, "crows")) { *out = popcount32(sv.crows); return 1; }
    if (!strcmp(key, "strays")) { *out = popcount32(sv.strays); return 1; }
    if (!strcmp(key, "abilities")) { *out = sv.abilities; return 1; }
    if (!strcmp(key, "pieces")) { *out = sv.pieces; return 1; }
    if (!strcmp(key, "placed")) { *out = sv.placed; return 1; }
    if (!strcmp(key, "won")) { *out = sv.won; return 1; }
    if (!strcmp(key, "won_all")) { *out = sv.won_all; return 1; }
    if (!strcmp(key, "checkpoint")) { *out = sv.checkpoint; return 1; }
    if (!strcmp(key, "pins")) { *out = popcount32(sv.pins); return 1; }
    if (!strcmp(key, "runs")) { *out = sv.runs; return 1; }
    if (!strcmp(key, "has_save")) { Save tmp; *out = game_save_read(game_current_index(), &tmp, (int)sizeof tmp) == (int)sizeof tmp; return 1; }
    if (!strcmp(key, "secrets")) { *out = sv.secrets; return 1; }
    if (!strcmp(key, "plates")) { *out = plates_lit; return 1; }
    if (!strcmp(key, "opened")) { *out = ctx.opened; return 1; }
    if (!strcmp(key, "badger")) { *out = bd.state; return 1; }
    if (!strcmp(key, "badger_hp")) { *out = bd.hp; return 1; }
    if (!strcmp(key, "badger_shots")) { *out = bd.shots_left; return 1; }
    if (!strcmp(key, "badger_spot")) { *out = bd.spot; return 1; }
    if (!strcmp(key, "cross")) { *out = bd.cross_on; return 1; }
    if (!strcmp(key, "strays_used")) { *out = strays_used; return 1; }
    if (!strcmp(key, "tile")) { *out = (unsigned char)lnk_tile(&ctx, ball.layer, (int)(ball.x / LNK_T), (int)(ball.y / LNK_T)); return 1; }
    if (!strncmp(key, "count_", 6)) { *out = lnk_count(atoi(key + 6)); return 1; }
    if (!strncmp(key, "crow_", 5)) {
        int id = atoi(key + 5);
        *out = -1;
        for (int i = 0; i < n_movers; i++)
            if (movers[i].kind == EK_CROW && movers[i].id == id) *out = movers[i].state * 10 + crow_countdown[id];
        return 1;
    }
    if (!strncmp(key, "slicer_", 7)) {
        int id = atoi(key + 7);
        *out = -1;
        for (int i = 0; i < n_movers; i++)
            if (movers[i].kind == EK_SLICER && movers[i].id == id) *out = movers[i].state;
        return 1;
    }
    /* reach_KIND_AB_OPEN: how many things of a kind can be reached from where
     * Dimple wakes with these abilities (and gates) */
    int kind, ab, op, id;
    if (sscanf(key, "reachthing_%d_%d_%d_%d", &kind, &id, &ab, &op) == 4) {
        /* can one thing (kind, id) be reached from the start with these? */
        LnkCtx c = ctx;
        c.abilities = (uint8_t)ab;
        c.opened = (uint8_t)op;
        uint8_t keep[LNK_LAYERS][LNK_MH][LNK_MW];
        memcpy(keep, tiles, sizeof keep);
        lnk_tiles_reset(tiles);
        const LnkThing *s = lnk_find(EK_START, 0), *t = lnk_find(kind, id);
        lnk_flow_from(&reach, &c, s->layer, s->tx, s->ty);
        *out = t && reach.d[t->layer][t->ty][t->tx] != LNK_FAR;
        memcpy(tiles, keep, sizeof keep);
        return 1;
    }
    if (sscanf(key, "reach_%d_%d_%d", &kind, &ab, &op) == 3) {
        LnkCtx c = ctx;
        c.abilities = (uint8_t)ab;
        c.opened = (uint8_t)op;
        uint8_t keep[LNK_LAYERS][LNK_MH][LNK_MW];
        memcpy(keep, tiles, sizeof keep);
        lnk_tiles_reset(tiles);
        const LnkThing *s = lnk_find(EK_START, 0);
        lnk_flow_from(&reach, &c, s->layer, s->tx, s->ty);
        *out = count_reach(&reach, kind);
        memcpy(tiles, keep, sizeof keep);
        return 1;
    }
    /* holes_unpaired: holes with no hole under (or over) them */
    if (!strcmp(key, "holes_unpaired")) {
        int bad = 0;
        for (int y = 0; y < LNK_MH; y++)
            for (int x = 0; x < LNK_MW; x++) {
                char a = LNK_MAP[0][y][x], b = LNK_MAP[1][y][x];
                bool ha = a == 'o' || a == 'O', hb = b == 'o' || b == 'O';
                if (ha != hb) { bad++; printf("  unpaired hole at %d,%d\n", x, y); }
            }
        *out = bad;
        return 1;
    }
    return 0;
}

static void give_all_upto(int wp) {
    /* set the save as the demo player would have it after the route's first wp steps */
    for (int i = 0; i < wp && i < ARRAY_LEN(BOT_ROUTE); i++) {
        const Wp *w = &BOT_ROUTE[i];
        switch (w->kind) {
        case EK_PIN: sv.checkpoint = w->id; sv.pins |= (uint16_t)(1u << w->id); break;
        case EK_IRON: sv.irons |= 1u << w->id; break;
        case EK_CROW: sv.crows |= (uint16_t)(1u << w->id); break;
        case EK_STRAY: sv.strays |= (uint8_t)(1u << w->id); break;
        case EK_ABILITY: sv.abilities |= (uint8_t)(1u << w->id); break;
        case EK_PIECE: sv.pieces |= (uint8_t)(1u << w->id); break;
        case EK_ALTAR: sv.placed = 1; break;
        default: break;
        }
    }
}

static int lnk_cheat(const char *cmd) {
    int a = 0, b = 0, c = 0;
    if (!strcmp(cmd, "newgame")) { new_game(); state = ST_PLAY; game_set_pausable(true); return 1; }
    if (sscanf(cmd, "route %d", &a) == 1) {
        /* start a run as the demo player would be after a waypoints */
        memset(&sv, 0, sizeof sv);
        sv.checkpoint = 0xFF;
        give_all_upto(a);
        save_now();
        start_run();
        game_set_pausable(true);
        bot_wp = a;
        bot_have = false;
        return 1;
    }
    if (sscanf(cmd, "ball %d %d %d", &a, &b, &c) == 3) {
        /* put the ball at rest on tile (b, c) of layer a */
        place_ball(a, b * LNK_T + LNK_T / 2.0f, c * LNK_T + LNK_T / 2.0f);
        char ch = lnk_tile(&ctx, a, b, c);
        ball.lie = ch == 's' && !has_ab(AB_TREAD) ? LIE_SAND : ch == 'u' ? LIE_DIVOT : (ch == 'o' || ch == 'O') ? LIE_CUP : LIE_GROUND;
        ball.slie = ball.lie;
        cam_x = ball.x - SCREEN_W / 2;
        cam_y = ball.y - SCREEN_H / 2;
        if (state != ST_PLAY) state = ST_PLAY;
        for (int i = 0; i < lnk_nthings; i++) touch_armed[i] = true;
        return 1;
    }
    if (sscanf(cmd, "ballpx %d %d %d", &a, &b, &c) == 3) { place_ball(a, (float)b, (float)c); return 1; }
    if (sscanf(cmd, "strokes %d", &a) == 1) { strokes = a; return 1; }
    if (sscanf(cmd, "abilities %d", &a) == 1) { sv.abilities = (uint8_t)a; sync_ctx(); return 1; }
    if (sscanf(cmd, "irons %d", &a) == 1) { sv.irons = (uint32_t)a; return 1; }
    if (sscanf(cmd, "pieces %d", &a) == 1) { sv.pieces = (uint8_t)a; return 1; }
    if (sscanf(cmd, "strays %d", &a) == 1) { sv.strays = (uint8_t)a; return 1; }
    if (sscanf(cmd, "crows %d", &a) == 1) { sv.crows = (uint16_t)a; return 1; }
    if (sscanf(cmd, "placed %d", &a) == 1) { sv.placed = (uint8_t)a; sync_ctx(); return 1; }
    if (sscanf(cmd, "plates %d", &a) == 1) { plates_lit = (uint8_t)a; sync_ctx(); return 1; }
    if (sscanf(cmd, "aim %d", &a) == 1) { aim = a % LNK_DIRS; return 1; }
    if (sscanf(cmd, "shoot %d %d", &a, &b) == 2) {
        /* a stroke as if aimed and charged (the rules after the swing) */
        aim = a % LNK_DIRS;
        level_shown = b;
        shoot();
        return 1;
    }
    if (sscanf(cmd, "checkpoint %d", &a) == 1) { sv.checkpoint = (uint8_t)a; return 1; }
    if (!strcmp(cmd, "save")) { save_now(); return 1; }
    if (sscanf(cmd, "mapview %d", &a) == 1) { map_view = a; return 1; }
    if (sscanf(cmd, "simshot %d %d", &a, &b) == 2) {
        /* print where a stroke from here would end (planning rules) */
        LnkBall e = ball;
        LnkCtx pc = ctx;
        pc.probe = true;
        bool touched;
        int ev = lnk_sim(&e, &pc, a, b, -1, -1, 0, -999, -999, 1, &touched);
        printf("  dir %d level %d: ev %d, layer %d at %.1f,%.1f (tile %d,%d)\n", a, b, ev, e.layer, e.x, e.y, (int)(e.x / 16),
               (int)(e.y / 16));
        return 1;
    }
    if (!strcmp(cmd, "respawn")) { start_run(); return 1; }
    int d = 0;
    if (sscanf(cmd, "slicer %d %d %d", &a, &b, &d) == 3) {
        /* move slicer a to hover over tile (b, d) of its layer */
        for (int i = 0; i < n_movers; i++)
            if (movers[i].kind == EK_SLICER && movers[i].id == a) {
                movers[i].x = movers[i].hx = b * LNK_T + LNK_T / 2.0f;
                movers[i].y = movers[i].hy = d * LNK_T + LNK_T / 2.0f;
                movers[i].state = SL_IDLE;
                movers[i].t = 0;
            }
        return 1;
    }
    if (!strcmp(cmd, "no_slicers")) { for (int i = 0; i < n_movers; i++) if (movers[i].kind == EK_SLICER) movers[i].state = SL_GONE; return 1; }
    if (sscanf(cmd, "badger %d %d %d", &a, &b, &c) >= 2) {
        bd.state = a;
        bd.hp = b;
        bd.t = 0;
        bd.shots_left = BADGER_SHOTS;
        if (c >= 0 && c < 5) bd.spot = c;
        bd.cross_on = false;
        return 1;
    }
    if (sscanf(cmd, "botwp %d", &a) == 1) { bot_wp = a; bot_have = false; return 1; }
    if (!strcmp(cmd, "things")) {
        static const char *K[EK_KINDS] = {"start", "pin", "iron", "crow", "stray", "npc", "lark", "alba", "slicer",
                                          "ability", "piece", "altar", "plate", "tree", "saucer", "den"};
        for (int i = 0; i < lnk_nthings; i++)
            printf("  %s %d: layer %d at %d,%d\n", K[lnk_things[i].kind], lnk_things[i].id, lnk_things[i].layer, lnk_things[i].tx,
                   lnk_things[i].ty);
        return 1;
    }
    if (sscanf(cmd, "reachmap %d %d", &a, &b) == 2) {
        /* print both layers, marking what can be reached from the start */
        static LnkFlow f;
        LnkCtx cc = ctx;
        cc.abilities = (uint8_t)a;
        cc.opened = (uint8_t)b;
        lnk_tiles_reset(tiles);
        const LnkThing *s = lnk_find(EK_START, 0);
        lnk_flow_from(&f, &cc, s->layer, s->tx, s->ty);
        for (int l = 0; l < LNK_LAYERS; l++) {
            printf("  layer %d\n", l);
            for (int y = 0; y < LNK_MH; y++) {
                printf("  ");
                for (int x = 0; x < LNK_MW; x++) {
                    char ch = LNK_MAP[l][y][x];
                    putchar(f.d[l][y][x] != LNK_FAR ? (lnk_is_marker(ch) ? ch : '+') : (lnk_is_marker(ch) ? '!' : ch));
                }
                printf("\n");
            }
        }
        for (int k = 0; k < EK_KINDS; k++) printf("  kind %d: %d of %d\n", k, count_reach(&f, k), lnk_count(k));
        return 1;
    }
    return 0;
}

const GameDef GAME_LOSTLINKS = {
    "lostlinks",
    "LOST LINKS",
    "1985",
    "ADVENTURE",
    "DIMPLE THE GOLF BALL WAKES UNDER THE OLD LINKS. EVERY ROLL IS A STROKE, AND STROKES ARE LIFE.",
    {"FIND THE HAMMERHEAD", "BEAT THE BRASS BADGER", "WIN WITH EVERYTHING FOUND"},
    GLYPH_DPAD "\tAIM\n"
    "HOLD " GLYPH_A "\tSWING (LET GO TO HIT)\n"
    GLYPH_B "\tCANCEL / HOLD TO LOOK\n"
    GLYPH_A " / " GLYPH_B " ROLLING\tHOP / BRAKE (ONCE FOUND)\n"
    "START\tPAUSE",
    C_LEAF, C_WHITE,
    lnk_load, lnk_start, lnk_update, lnk_draw, lnk_quit, lnk_label, lnk_query, lnk_cheat,
    "GOLFARIA", 18,
};
