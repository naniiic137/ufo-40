/* HAT TRICK - the rules of one screen: the kids, the ball, the chain, the
 * food, the clock and extra time. Everything lives in one HtkPlay so the
 * demo player can copy a screen whole and look ahead; nothing here draws,
 * and sound goes through htk_sfx (quiet while it looks ahead). */
#include "hattrick.h"

HtkPlay htk;
bool htk_sim;

const int HTK_ITEM_VALUE[IT_COUNT] = {100, 200, 500, 1000, 100, 200, 500, 1000};
const char *const HTK_ITEM_NAME[IT_COUNT] = {"POPCORN", "PRETZEL", "TACO", "DRUMSTICK",
                                             "LOLLY", "COOKIE", "DONUT", "PARFAIT"};
static const uint32_t EXTENDS[] = {10000, 25000, 50000, 75000, 100000};

void htk_sfx(const char *name) {
    if (!htk_sim) sfx_play_name(name);
}

/* ---- the grid, which wraps wherever there is no wall ------------------------ */

int htk_tile(const HtkPlay *g, float x, float y) {
    int tx = (int)floorf(x / HTK_T), ty = (int)floorf(y / HTK_T);
    tx %= HTK_COLS;
    if (tx < 0) tx += HTK_COLS;
    ty %= HTK_ROWS;
    if (ty < 0) ty += HTK_ROWS;
    return g->tile[ty][tx];
}

float htk_wrapdx(float d) {
    while (d > HTK_W / 2) d -= HTK_W;
    while (d < -HTK_W / 2) d += HTK_W;
    return d;
}
float htk_wrapdy(float d) {
    while (d > HTK_H / 2) d -= HTK_H;
    while (d < -HTK_H / 2) d += HTK_H;
    return d;
}

float htk_wrapx(float x) {
    while (x < 0) x += HTK_W;
    while (x >= HTK_W) x -= HTK_W;
    return x;
}

static bool kid_solid(int t) { return t == T_SOLID || t == T_GOAL; }

/* The top of the first ground tile the span [xl, xr] crosses as its bottom
 * goes from oldy down to newy, or -1e9. Ledges only count from above. */
float htk_find_floor(const HtkPlay *g, float xl, float xr, float oldy, float newy, bool kid) {
    if (newy < oldy) return -1e9f;
    int r0 = (int)ceilf(oldy / HTK_T - 0.0001f), r1 = (int)floorf(newy / HTK_T);
    for (int r = r0; r <= r1; r++) {
        float top = (float)(r * HTK_T);
        for (float x = xl; x <= xr + 0.01f; x += 4.0f) {
            float xx = x > xr ? xr : x;
            int t = htk_tile(g, xx, top + 1);
            if (t == T_LEDGE || t == T_SOLID || (kid && t == T_GOAL)) {
                /* a solid tile counts only if the space above it is open (not
                 * a tile buried in a wall) */
                return top;
            }
        }
    }
    return -1e9f;
}

/* A solid tile across [xl, xr] in the row where the head now is, whose
 * bottom the head has just passed going up. Returns that bottom, or -1e9. */
static float find_ceiling(const HtkPlay *g, float xl, float xr, float oldtop, float newtop, bool kid) {
    if (newtop > oldtop) return -1e9f;
    int r = (int)floorf(newtop / HTK_T);
    float bottom = (float)(r * HTK_T + HTK_T);
    if (bottom > oldtop + 0.01f) return -1e9f;
    for (float x = xl; x <= xr + 0.01f; x += 4.0f) {
        float xx = x > xr ? xr : x;
        int t = htk_tile(g, xx, newtop);
        if (kid ? kid_solid(t) : t == T_SOLID) return bottom;
    }
    return -1e9f;
}

/* Is any solid tile in the box? */
bool htk_box_solid(const HtkPlay *g, float x0, float y0, float x1, float y1, bool kid) {
    for (float y = y0; y <= y1 + 0.01f; y += 4.0f) {
        float yy = y > y1 ? y1 : y;
        for (float x = x0; x <= x1 + 0.01f; x += 4.0f) {
            float xx = x > x1 ? x1 : x;
            int t = htk_tile(g, xx, yy);
            if (kid ? kid_solid(t) : t == T_SOLID) return true;
        }
    }
    return false;
}

/* ---- little things ----------------------------------------------------------- */

void htk_burst(HtkPlay *g, float x, float y, int col, int n, float sp) {
    if (htk_sim) return;
    for (int k = 0; k < n; k++)
        for (int i = 0; i < HTK_MAX_PARTS; i++) {
            HtkPart *p = &g->part[i];
            if (p->life > 0) continue;
            float a = rng_float(&g_rng) * 6.2831853f, s = sp * (0.4f + rng_float(&g_rng) * 0.8f);
            *p = (HtkPart){x, y, cosf(a) * s, sinf(a) * s - 0.6f, (int16_t)(18 + rng_range(&g_rng, 0, 14)), (int16_t)col};
            break;
        }
}

static void pop_score(HtkPlay *g, float x, float y, int value) {
    int best = 0;
    for (int i = 0; i < HTK_MAX_POPS; i++)
        if (g->pop[i].t < g->pop[best].t) best = i;
    g->pop[best] = (HtkPop){x, y, 50, (int16_t)value};
}

void htk_add_score(HtkPlay *g, int i, int pts) {
    HtkPlayer *p = &g->pl[i];
    p->score += (uint32_t)pts;
    while (p->next_ext < ARRAY_LEN(EXTENDS) && p->score >= EXTENDS[p->next_ext]) {
        p->next_ext++;
        p->spare++;
        htk_sfx("htk_extend");
    }
}

int htk_foes_left(const HtkPlay *g) {
    int n = 0;
    for (int i = 0; i < HTK_MAX_FOES; i++) n += g->foe[i].alive && g->foe[i].kind != FK_TIMEKEEPER;
    return n;
}

int htk_counts_left(const HtkPlay *g) { return (g->clock + HTK_TICK - 1) / HTK_TICK; }

float htk_ball_speed(const HtkPlay *g) { return fabsf(g->ball.vx) + fabsf(g->ball.vy); }

/* ---- loading a screen -------------------------------------------------------- */

static const int BOSS_OF_WORLD[HTK_WORLDS] = {FK_KINGSPIKER, FK_KINGPIN, FK_LIFEGUARD, FK_CAPTAIN};

static void place_kid(HtkPlay *g, int i, int side, int lx, int ly) {
    HtkPlayer *p = &g->pl[i];
    p->start_x = (int16_t)(side ? HTK_W - lx : lx);
    p->start_y = (int16_t)ly;
    p->start_face = (int16_t)(side ? -1 : 1);
    p->x = p->start_x;
    p->y = p->start_y;
    p->vx = p->vy = 0;
    p->facing = p->start_face;
    p->alive = !p->out;
    p->dead_t = p->inv = p->slide_t = p->head_t = p->kick_t = p->charge = 0;
    p->charging = p->crouch = p->lifted = 0;
    p->ground = 1;
}

void htk_load_level(HtkPlay *g, int level) {
    const HtkLevelDef *d = level < 0 ? &HTK_VS_PITCH : &HTK_LEVEL[level];
    int world = level < 0 ? 0 : level / HTK_PER_WORLD;
    g->level = level;
    memset(g->foe, 0, sizeof g->foe);
    memset(g->shot, 0, sizeof g->shot);
    memset(g->item, 0, sizeof g->item);
    memset(g->part, 0, sizeof g->part);
    memset(g->pop, 0, sizeof g->pop);
    g->dessert_tx = g->dessert_ty = -1;
    g->dessert_found = 0;
    g->boss_kind = -1;
    int sx = 24, sy = 160;
    float bx = 160, by = 80;
    for (int r = 0; r < HTK_ROWS; r++) {
        const char *row = d->rows[r];
        for (int c = 0; c < HTK_HALF; c++) {
            char ch = row[c];
            int t = ch == '#' ? T_SOLID : ch == '=' ? T_LEDGE : ch == 'G' ? T_GOAL : T_EMPTY;
            g->tile[r][c] = g->tile[r][HTK_COLS - 1 - c] = (uint8_t)t;
            float x = (float)(c * HTK_T + 4), y = (float)(r * HTK_T + 4);
            if (ch == 'P') { sx = c * HTK_T + 4; sy = (r + 1) * HTK_T; }
            else if (ch == 'O') { bx = c == HTK_HALF - 1 ? HTK_W / 2 : x; by = (float)((r + 1) * HTK_T - HTK_BALL_R); }
            else if (ch == '*') { g->dessert_tx = c; g->dessert_ty = r; }
            else if ((ch >= 'a' && ch <= 'c') || (ch >= 'A' && ch <= 'C')) {
                int lower = ch >= 'a' ? ch - 'a' : ch - 'A';
                int alt = ch < 'a';
                int kind = world * 3 + lower;
                htk_spawn_foe(g, kind, x, y, 1, alt);
                htk_spawn_foe(g, kind, HTK_W - x, y, -1, alt);
            } else if (ch == 'X' || ch == 'Y') {
                int kind = ch == 'Y' ? FK_TOWER : BOSS_OF_WORLD[world];
                if (ch == 'X') g->boss_kind = kind;
                htk_spawn_foe(g, kind, c == HTK_HALF - 1 ? HTK_W / 2 : x, y, 1, 0);
            }
        }
    }
    /* who starts where: in 1P the kid picked chooses the side */
    if (g->mode == MODE_1P) place_kid(g, 0, g->pl[0].ch == KID_MAE, sx, sy);
    else {
        place_kid(g, 0, 0, sx, sy);
        place_kid(g, 1, 1, sx, sy);
    }
    memset(&g->ball, 0, sizeof g->ball);
    g->ball.x = bx;
    g->ball.y = by;
    g->ball.carrier = g->ball.last = -1;
    g->ball.ground = 1;
    g->combo = 0;
    g->clock = HTK_COUNTS * HTK_TICK;
    g->overtime = g->extra_t = 0;
    g->bonus = 0;
    g->sub = level < 0 ? LS_PLAY : LS_INTRO;
    g->sub_t = 0;
    g->clear_wait = 0;
    g->ev_exit = g->ev_dead = 0;
}

/* ---- the kids ----------------------------------------------------------------- */

static float kid_h(const HtkPlayer *p) { return (p->crouch || p->slide_t > 0) ? HTK_PH_LOW : HTK_PH; }

static void move_kid(HtkPlay *g, HtkPlayer *p) {
    float h = kid_h(p);
    /* across */
    p->x += p->vx;
    if (p->vx > 0) {
        float lead = p->x + HTK_PW - 0.01f;
        if (htk_box_solid(g, lead, p->y - h, lead, p->y - 0.01f, true)) {
            p->x = floorf(lead / HTK_T) * HTK_T - HTK_PW;
            p->vx = 0;
            p->slide_t = 0;
        }
    } else if (p->vx < 0) {
        float lead = p->x - HTK_PW;
        if (htk_box_solid(g, lead, p->y - h, lead, p->y - 0.01f, true)) {
            p->x = floorf(lead / HTK_T) * HTK_T + HTK_T + HTK_PW;
            p->vx = 0;
            p->slide_t = 0;
        }
    }
    p->x = htk_wrapx(p->x);
    /* up and down */
    float oldy = p->y;
    p->y += p->vy;
    p->ground = 0;
    if (p->vy >= 0) {
        float top = htk_find_floor(g, p->x - HTK_PW + 1, p->x + HTK_PW - 1, oldy, p->y, true);
        if (top > -1e8f) {
            p->y = top;
            p->vy = 0;
            p->ground = 1;
        }
    } else {
        float b = find_ceiling(g, p->x - HTK_PW + 1, p->x + HTK_PW - 1, oldy - h, p->y - h, true);
        if (b > -1e8f) {
            p->y = b + h;
            p->vy = 0;
        }
    }
    if (p->y >= HTK_H) p->y -= HTK_H;
    if (p->y < 0) p->y += HTK_H;
}

/* The kick: the pad's direction, the kid's run and jump, and a charge. */
static void kick(HtkPlay *g, int i, uint16_t held, bool charged) {
    HtkPlayer *p = &g->pl[i];
    HtkBall *b = &g->ball;
    int dx = ((held & BTN_RIGHT) != 0) - ((held & BTN_LEFT) != 0);
    bool up = held & BTN_UP, down = held & BTN_DOWN;
    if (dx) p->facing = (int16_t)dx;
    float f = p->facing, vx, vy;
    if (up && !dx) { vx = 0.9f * f; vy = -4.6f; }          /* a lob, nearly straight up */
    else if (up) { vx = 2.4f * f; vy = -3.6f; }           /* up and over */
    else if (down && !p->ground) { vx = (dx ? 2.4f : 1.2f) * f; vy = 3.2f; } /* a volley down */
    else if (down) { vx = 3.4f * f; vy = -0.3f; }         /* along the ground */
    else if (dx) { vx = 3.3f * f; vy = -1.6f; }           /* a low drive on the run */
    else { vx = 2.6f * f; vy = -2.6f; }                   /* a chip from standing */
    vx += p->vx * 0.5f;
    if (!p->ground) vy += p->vy * 0.4f;
    b->power_t = 0;
    if (charged) {
        vx *= 1.6f;
        vy *= 0.55f;
        b->power_t = HTK_POWER_T;
    }
    b->carrier = -1;
    b->x = htk_wrapx(p->x + f * 8);
    b->y = p->y - (p->ground ? 4 : 8);
    if (htk_box_solid(g, b->x - HTK_BALL_R, b->y - HTK_BALL_R, b->x + HTK_BALL_R, b->y + HTK_BALL_R, false)) {
        b->x = p->x;
        if (htk_box_solid(g, b->x - HTK_BALL_R, b->y - HTK_BALL_R, b->x + HTK_BALL_R, b->y + HTK_BALL_R, false)) b->y = p->y - 12;
    }
    b->vx = vx;
    b->vy = vy;
    b->lit = 1;
    b->hold_t = b->bounces = b->rest_t = 0;
    b->ground = 0;
    b->last = (int16_t)i;
    b->nopick[i] = HTK_KICK_CD;
    p->kick_t = 12;
    htk_sfx(charged ? "htk_drive" : "htk_kick");
}

/* a touch without carrying: the slide and the header */
static void touch_ball(HtkPlay *g, int i, float vx, float vy) {
    HtkBall *b = &g->ball;
    b->vx = vx;
    b->vy = vy;
    b->lit = 1;
    b->bounces = b->rest_t = 0;
    b->ground = 0;
    b->power_t = 0;
    b->last = (int16_t)i;
    b->nopick[i] = HTK_KICK_CD;
    htk_sfx("htk_touch");
}

void htk_kill_player(HtkPlay *g, int i) {
    HtkPlayer *p = &g->pl[i];
    if (!p->alive || g->god) return;
    p->alive = 0;
    p->deaths++;
    p->dead_t = 0;
    p->slide_t = p->head_t = p->charge = p->charging = 0;
    p->vy = -3.0f;
    if (g->ball.carrier == i) {
        g->ball.carrier = -1;
        g->ball.vx = 0;
        g->ball.vy = -2.0f;
        g->ball.ground = 0;
        g->ball.nopick[0] = g->ball.nopick[1] = 30;
    }
    htk_sfx("htk_hurt");
}

static void respawn(HtkPlay *g, int i) {
    HtkPlayer *p = &g->pl[i];
    p->x = p->start_x;
    p->y = p->start_y;
    p->vx = p->vy = 0;
    p->facing = p->start_face;
    p->alive = 1;
    p->inv = HTK_INV_T;
    p->dead_t = 0;
    p->ground = 1;
}

static void update_kid(HtkPlay *g, int i, uint16_t held) {
    HtkPlayer *p = &g->pl[i];
    HtkBall *b = &g->ball;
    uint16_t pressed = held & (uint16_t)~p->prev, released = p->prev & (uint16_t)~held;
    p->prev = held;
    if (!p->on || p->out) return;
    p->anim++;
    if (!p->alive) {
        p->dead_t++;
        /* a little tumble where it happened */
        if (p->dead_t < 40) {
            p->vy = fminf(p->vy + HTK_GRAV, HTK_MAXFALL);
            p->y += p->vy;
            if (p->y >= HTK_H) p->y -= HTK_H;
            if (p->y < 0) p->y += HTK_H;
        }
        if (p->dead_t == HTK_DEATH_T) {
            if (g->mode == MODE_VS) respawn(g, i);
            else if (p->spare > 0) {
                p->spare--;
                respawn(g, i);
            } else p->out = 1;
        }
        return;
    }
    if (p->inv > 0) p->inv--;
    if (p->kick_t > 0) p->kick_t--;
    if (p->lifted || g->sub == LS_INTRO || g->sub == LS_GOAL) {
        p->vx = 0;
        if (g->sub == LS_INTRO || g->sub == LS_GOAL) {
            p->vy = fminf(p->vy + HTK_GRAV, HTK_MAXFALL);
            move_kid(g, p);
        }
        return;
    }
    int dx = ((held & BTN_RIGHT) != 0) - ((held & BTN_LEFT) != 0);
    bool carrying = b->carrier == i;
    p->crouch = p->ground && (held & BTN_DOWN) && p->slide_t == 0;
    if (p->slide_t > 0) {
        /* the slide keeps its line, gaps or not */
        p->slide_t--;
        p->vx = p->facing * HTK_SLIDE_V;
        p->vy = 0;
    } else {
        float target = p->crouch ? 0 : dx * HTK_RUN;
        p->vx = fapproach(p->vx, target, p->ground ? HTK_GROUND_ACC : HTK_AIR_ACC);
        if (dx) p->facing = (int16_t)dx;
        if ((pressed & BTN_A) && p->ground && !p->crouch) {
            p->vy = HTK_JUMP;
            p->ground = 0;
            htk_sfx("htk_jump");
        }
        p->vy = fminf(p->vy + HTK_GRAV, HTK_MAXFALL);
    }
    move_kid(g, p);
    if (p->head_t > 0) p->head_t--;
    if (carrying) {
        if (p->ground && (pressed & BTN_B)) {
            p->charging = 1;
            p->charge = 0;
        } else if (!p->ground && (pressed & BTN_B)) {
            kick(g, i, held, false);
            p->charging = 0;
        }
        if (p->charging && b->carrier == i) {
            if (held & BTN_B) {
                if (p->ground) p->charge++;
            } else if (released & BTN_B) {
                kick(g, i, held, p->charge >= HTK_CHARGE_T);
                p->charging = 0;
                p->charge = 0;
            }
        }
    } else {
        p->charging = 0;
        p->charge = 0;
        if (pressed & BTN_B) {
            if (p->ground && p->slide_t == 0) {
                p->slide_t = HTK_SLIDE_T;
                p->crouch = 0;
                htk_sfx("htk_slide");
            } else if (!p->ground && p->head_t == 0) {
                p->head_t = HTK_HEAD_T;
            }
        }
    }
    /* the ball: a slide or a header knocks it on, walking into it on the ground picks it up */
    if (b->carrier < 0 && g->sub != LS_EXIT) {
        float bdx = htk_wrapdx(b->x - p->x), bdy = htk_wrapdy(b->y - p->y);
        float h = kid_h(p);
        bool body = fabsf(bdx) < HTK_PW + HTK_BALL_R && bdy > -h - HTK_BALL_R && bdy < HTK_BALL_R;
        if (p->slide_t > 0 && body) {
            touch_ball(g, i, p->facing * 3.2f, -2.0f);
        } else if (p->head_t > 0 && fabsf(bdx) < HTK_PW + 4 && bdy > -h - 8 && bdy < -h + 9) {
            touch_ball(g, i, p->facing * 1.8f + dx * 1.0f, -3.4f);
            p->head_t = 0;
        } else if (p->ground && body && b->nopick[i] == 0) {
            b->carrier = (int16_t)i;
            b->last = (int16_t)i;
            b->lit = 1;
            b->hold_t = 0;
            b->bounces = b->rest_t = 0;
            b->power_t = 0;
            htk_sfx("htk_trap");
        }
    }
    /* versus: a slide into the kid with the ball knocks it loose */
    if (g->mode == MODE_VS && p->slide_t > 0 && b->carrier == 1 - i) {
        const HtkPlayer *o = &g->pl[1 - i];
        if (fabsf(htk_wrapdx(o->x - p->x)) < HTK_PW * 2 && fabsf(htk_wrapdy(o->y - p->y)) < 10) {
            b->carrier = -1;
            touch_ball(g, i, p->facing * 2.4f, -2.6f);
            g->pl[1 - i].charging = 0;
        }
    }
}

/* ---- the ball ------------------------------------------------------------------ */

static void go_dark(HtkPlay *g) {
    if (!g->ball.lit || g->mode == MODE_VS) return;
    g->ball.lit = 0;
    g->combo = 0;
    htk_sfx("htk_dark");
}

static void update_ball(HtkPlay *g) {
    HtkBall *b = &g->ball;
    for (int k = 0; k < 2; k++)
        if (b->nopick[k] > 0) b->nopick[k]--;
    if (b->power_t > 0) b->power_t--;
    if (b->carrier >= 0) {
        HtkPlayer *p = &g->pl[b->carrier];
        b->x = htk_wrapx(p->x + p->facing * 7);
        b->y = p->y - HTK_BALL_R - 1;
        if (b->y < 0) b->y += HTK_H;
        b->vx = b->vy = 0;
        b->spin += (int16_t)(fabsf(p->vx) > 0.2f ? 1 : 0);
        if (++b->hold_t > HTK_DARK_HOLD) go_dark(g);
        return;
    }
    float sp = fmaxf(fabsf(b->vx), fabsf(b->vy));
    int n = 1 + (int)(sp / 2.5f);
    bool landed = false;
    for (int s = 0; s < n; s++) {
        float ox = b->x;
        b->x += b->vx / n;
        if (htk_box_solid(g, b->x - HTK_BALL_R, b->y - HTK_BALL_R + 1, b->x + HTK_BALL_R, b->y + HTK_BALL_R - 1, false)) {
            b->x = ox;
            b->vx = -b->vx * 0.75f;
            if (fabsf(b->vx) > 0.6f) htk_sfx("htk_bonk");
        }
        b->x = htk_wrapx(b->x);
        float oy = b->y;
        b->y += b->vy / n;
        if (b->vy >= 0) {
            float top = htk_find_floor(g, b->x - HTK_BALL_R + 1, b->x + HTK_BALL_R - 1, oy + HTK_BALL_R, b->y + HTK_BALL_R, false);
            if (top > -1e8f) {
                b->y = top - HTK_BALL_R;
                if (b->vy > 0.9f) {
                    b->vy = -b->vy * 0.6f;
                    if (b->lit && ++b->bounces >= HTK_DARK_BOUNCES) go_dark(g);
                    htk_sfx("htk_bounce");
                } else {
                    b->vy = 0;
                    landed = true;
                }
            }
        } else {
            float bot = find_ceiling(g, b->x - HTK_BALL_R + 1, b->x + HTK_BALL_R - 1, oy - HTK_BALL_R, b->y - HTK_BALL_R, false);
            if (bot > -1e8f) {
                b->y = bot + HTK_BALL_R;
                b->vy = -b->vy * 0.5f;
            }
        }
        if (b->y >= HTK_H) b->y -= HTK_H;
        if (b->y < 0) b->y += HTK_H;
    }
    b->ground = landed;
    if (landed) {
        b->vx *= 0.97f;
        if (fabsf(b->vx) < 0.05f) b->vx = 0;
        if (b->lit && ++b->rest_t >= HTK_DARK_REST) go_dark(g);
    }
    b->vy = fminf(b->vy + HTK_BALL_GRAV, 5.0f);
    b->spin += (int16_t)(fabsf(b->vx) * 2);
    /* the hidden dessert spot: kick the ball through it */
    if (g->dessert_tx >= 0 && !g->dessert_found && htk_ball_speed(g) > 0.5f) {
        float cx = g->dessert_tx * HTK_T + 4.0f, cy = g->dessert_ty * HTK_T + 4.0f;
        if (fabsf(htk_wrapdx(b->x - cx)) < 6 && fabsf(htk_wrapdy(b->y - cy)) < 6) {
            g->dessert_found = 1;
            int world = g->level / HTK_PER_WORLD;
            htk_spawn_body(g, cx, cy, -1, IT_LOLLY + world, 0);
            htk_burst(g, cx, cy, C_PINK, 10, 1.4f);
            htk_sfx("htk_secret");
        }
    }
}

/* ---- the ball against the creatures --------------------------------------------- */

static void ball_vs_foes(HtkPlay *g) {
    HtkBall *b = &g->ball;
    if (b->carrier >= 0 || htk_ball_speed(g) < HTK_KILL_SPEED) return;
    for (int i = 0; i < HTK_MAX_FOES; i++) {
        HtkFoe *f = &g->foe[i];
        if (!f->alive || f->away || f->kind == FK_TIMEKEEPER) continue;
        float dx = htk_wrapdx(b->x - f->x), dy = htk_wrapdy(b->y - f->y);
        if (fabsf(dx) > f->hw + HTK_BALL_R + 1 || fabsf(dy) > f->hh + HTK_BALL_R + 1) continue;
        if (f->kind >= FK_KINGSPIKER) {
            if (f->flash > 0) continue;
            htk_boss_hit(g, i, b->power_t > 0 ? 3 : 1);
            /* the ball comes back off a boss */
            b->vx = (dx >= 0 ? 1 : -1) * fmaxf(fabsf(b->vx) * 0.7f, 1.4f);
            b->vy = -fabsf(b->vy) * 0.5f - 1.6f;
            b->power_t = 0;
            continue;
        }
        /* a kill: the chain sets what it drops */
        int item = IT_POPCORN;
        if (b->lit) {
            g->combo++;
            item = imin(g->combo, 4) - 1;
        }
        f->alive = 0;
        g->kills++;
        if (b->last >= 0) g->pl[b->last].kills++;
        htk_spawn_body(g, f->x, f->y, f->kind, item, b->vx > 0 ? 0.6f : -0.6f);
        htk_burst(g, f->x, f->y, C_WHITE, 6, 1.2f);
        htk_sfx(item >= IT_DRUMSTICK ? "htk_kill4" : "htk_kill");
        b->vx *= 0.92f;
    }
}

/* ---- bodies and food -------------------------------------------------------------- */

static void update_items(HtkPlay *g) {
    for (int i = 0; i < HTK_MAX_ITEMS; i++) {
        HtkItem *it = &g->item[i];
        if (!it->alive) continue;
        it->t++;
        if (it->falling) {
            it->fall_t++;
            float oy = it->y;
            it->vy = fminf(it->vy + 0.15f, 3.5f);
            it->x = htk_wrapx(it->x + it->vx);
            it->y += it->vy;
            /* a body only lands on the way down, once it has left where it died */
            float top = it->vy > 0 ? htk_find_floor(g, it->x - 3, it->x + 3, oy + 4, it->y + 4, false) : -1e9f;
            if (top > -1e8f && it->fall_t > 6) {
                it->y = top - 4;
                it->falling = 0;
                it->body = -1;
                it->t = 0;
                htk_sfx("htk_land");
            }
            if (it->y >= HTK_H) it->y -= HTK_H;
            if (it->y < 0) it->y += HTK_H;
            continue;
        }
        /* the food: any kid walking into it eats it */
        for (int k = 0; k < 2; k++) {
            HtkPlayer *p = &g->pl[k];
            if (!p->on || !p->alive || p->lifted) continue;
            float dx = htk_wrapdx(it->x - p->x), dy = htk_wrapdy(it->y - (p->y - kid_h(p) / 2));
            if (fabsf(dx) < HTK_PW + 4 && fabsf(dy) < kid_h(p) / 2 + 4) {
                int v = HTK_ITEM_VALUE[it->item];
                htk_add_score(g, k, v);
                if (it->item >= IT_LOLLY) p->desserts++;
                pop_score(g, it->x, it->y - 6, v);
                htk_sfx(it->item >= IT_LOLLY ? "htk_dessert" : "htk_eat");
                it->alive = 0;
                break;
            }
        }
    }
}

static bool any_falling(const HtkPlay *g) {
    for (int i = 0; i < HTK_MAX_ITEMS; i++)
        if (g->item[i].alive && g->item[i].falling) return true;
    return false;
}

/* ---- touching danger --------------------------------------------------------------- */

static void kids_vs_danger(HtkPlay *g) {
    for (int k = 0; k < 2; k++) {
        HtkPlayer *p = &g->pl[k];
        if (!p->on || !p->alive || p->inv > 0 || p->lifted) continue;
        float h = kid_h(p), cy = p->y - h / 2;
        for (int i = 0; i < HTK_MAX_FOES; i++) {
            const HtkFoe *f = &g->foe[i];
            if (!f->alive || f->away) continue;
            if (fabsf(htk_wrapdx(f->x - p->x)) < HTK_PW + f->hw - 2 && fabsf(htk_wrapdy(f->y - cy)) < h / 2 + f->hh - 2) {
                p->hit_by = f->kind;
                htk_kill_player(g, k);
                break;
            }
        }
        if (!p->alive) continue;
        for (int i = 0; i < HTK_MAX_SHOTS; i++) {
            const HtkShot *s = &g->shot[i];
            if (!s->alive) continue;
            if (fabsf(htk_wrapdx(s->x - p->x)) < HTK_PW + 2 && fabsf(htk_wrapdy(s->y - cy)) < h / 2 + 2) {
                p->hit_by = (int16_t)(100 + s->kind);
                htk_kill_player(g, k);
                break;
            }
        }
    }
}

/* ---- the screen's flow ---------------------------------------------------------------- */

static void start_overtime(HtkPlay *g) {
    g->overtime = 1;
    g->extra_t = 150;
    /* the Timekeeper walks in at the top, as far from the kids as he can */
    float x = 160;
    int k = htk_nearest_player(g, 160, 84);
    if (k >= 0) x = g->pl[k].x < 160 ? 300 : 20;
    htk_spawn_foe(g, FK_TIMEKEEPER, x, 10, 1, 0);
    htk_sfx("htk_whistle");
}

static void clear_screen(HtkPlay *g) {
    g->sub = LS_CLEAR;
    g->sub_t = 0;
    g->bonus = g->overtime ? 0 : htk_counts_left(g) * HTK_BONUS;
    for (int k = 0; k < 2; k++)
        if (g->pl[k].on && !g->pl[k].out && g->bonus) htk_add_score(g, k, g->bonus);
    /* the Timekeeper and every shot go with the last creature */
    for (int i = 0; i < HTK_MAX_FOES; i++)
        if (g->foe[i].kind == FK_TIMEKEEPER) g->foe[i].alive = 0;
    memset(g->shot, 0, sizeof g->shot);
    htk_sfx("htk_clear");
}

static void update_versus(HtkPlay *g) {
    HtkBall *b = &g->ball;
    b->lit = 1;
    if (g->sub == LS_GOAL) {
        if (g->sub_t >= 100) {
            if (g->vs_score[g->vs_scorer] >= g->vs_target) {
                g->ev_vswin = 1;
                return;
            }
            for (int k = 0; k < 2; k++) {
                HtkPlayer *p = &g->pl[k];
                p->x = p->start_x;
                p->y = p->start_y;
                p->vx = p->vy = 0;
                p->facing = p->start_face;
                p->slide_t = p->head_t = 0;
            }
            memset(b, 0, sizeof *b);
            b->x = 160;
            b->y = 40;
            b->carrier = b->last = -1;
            b->lit = 1;
            g->sub = LS_PLAY;
            g->sub_t = 0;
        }
        return;
    }
    if (b->carrier < 0 && (b->x < 3 || b->x > HTK_W - 3) && htk_tile(g, b->x, b->y) == T_GOAL) {
        /* in the left goal: a point to the kid on the right, and the other way */
        g->vs_scorer = b->x < 160 ? 1 : 0;
        g->vs_score[g->vs_scorer]++;
        g->sub = LS_GOAL;
        g->sub_t = 0;
        b->vx = b->vy = 0;
        htk_sfx("htk_goal");
    }
}

void htk_step(HtkPlay *g, uint16_t pad0, uint16_t pad1) {
    g->frame++;
    g->sub_t++;
    if (g->extra_t > 0) g->extra_t--;
    for (int i = 0; i < HTK_MAX_PARTS; i++) {
        HtkPart *p = &g->part[i];
        if (p->life <= 0) continue;
        p->life--;
        p->x += p->vx;
        p->y += p->vy;
        p->vy += 0.08f;
    }
    for (int i = 0; i < HTK_MAX_POPS; i++)
        if (g->pop[i].t > 0) { g->pop[i].t--; g->pop[i].y -= 0.3f; }
    if (g->sub == LS_INTRO) {
        update_kid(g, 0, pad0);
        update_kid(g, 1, pad1);
        if (g->sub_t >= HTK_INTRO_T) { g->sub = LS_PLAY; g->sub_t = 0; }
        return;
    }
    if (g->sub == LS_EXIT) {
        for (int k = 0; k < 2; k++) {
            HtkPlayer *p = &g->pl[k];
            p->prev = k ? pad1 : pad0;
            if (!p->on || p->out || !p->alive) continue;
            if (g->sub_t > 30) p->y -= fminf((g->sub_t - 30) * 0.06f, 2.5f);
        }
        if (g->sub_t >= HTK_EXIT_T) g->ev_exit = 1;
        return;
    }
    update_kid(g, 0, pad0);
    update_kid(g, 1, pad1);
    update_ball(g);
    if (g->mode == MODE_VS) {
        update_versus(g);
        return;
    }
    htk_foes_update(g);
    ball_vs_foes(g);
    htk_shots_update(g);
    if (g->sub == LS_PLAY) kids_vs_danger(g);
    update_items(g);
    if (g->sub == LS_PLAY) {
        if (g->clock > 0 && --g->clock == 0) start_overtime(g);
        if (htk_foes_left(g) == 0) clear_screen(g);
    } else if (g->sub == LS_CLEAR) {
        /* the food keeps coming down; then a little time to pick it up */
        g->clear_wait++;
        if (any_falling(g) && g->clear_wait < 900) g->sub_t = 0;
        if (g->sub_t >= HTK_COLLECT_T) {
            g->sub = LS_EXIT;
            g->sub_t = 0;
            for (int k = 0; k < 2; k++) g->pl[k].lifted = 1;
            htk_sfx("htk_balloon");
        }
    }
    /* out of lives */
    bool any = false;
    for (int k = 0; k < 2; k++)
        if (g->pl[k].on && !g->pl[k].out) any = true;
    if (!any) g->ev_over = 1;
}
