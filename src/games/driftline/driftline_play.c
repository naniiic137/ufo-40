/* DRIFTLINE - the drive itself: the cars, their two guns and the charge
 * meter, shots and hits, cars lost and cars coming back, and the bonus
 * stage of blocks. The foes and bosses are in driftline_foes.c. */
#include "driftline.h"

DflGame dfg;
DflSave dfs;

const int DFL_TIER_DMG[3] = {1, 2, 4};
const int DFL_MAIN_CD[3] = {8, 6, 5};
const int DFL_SIDE_CD[3] = {12, 10, 8};
const float DFL_SHOT_SPEED[3] = {4.5f, 5.25f, 6.0f};

int dfl_tier(int meter) { return meter >= DFL_TIER2 ? 2 : meter >= DFL_TIER1 ? 1 : 0; }

static uint32_t sfx_last[16];
static const char *sfx_names[16];

/* a sound that fires often: played at most once every gap frames */
void dfl_sfx(const char *name, int gap) {
    uint32_t now = (uint32_t)dfg.frame_t;
    for (int i = 0; i < 16; i++) {
        if (sfx_names[i] == name) {
            if (now - sfx_last[i] < (uint32_t)gap && now >= sfx_last[i]) return;
            sfx_last[i] = now;
            sfx_play_name(name);
            return;
        }
        if (!sfx_names[i]) {
            sfx_names[i] = name;
            sfx_last[i] = now;
            sfx_play_name(name);
            return;
        }
    }
    sfx_play_name(name);
}

void dfl_add_score(uint32_t pts) {
    dfg.score += pts;
    if (dfg.score > 9999990u) dfg.score = 9999990u;
}

void dfl_note_cars(void) {
    if (dfg.spare > dfg.most_cars) dfg.most_cars = dfg.spare;
}

int dfl_cars_alive(void) {
    int n = 0;
    for (int p = 0; p < DFL_CARS; p++) n += dfg.car[p].on && dfg.car[p].alive;
    return n;
}

void dfl_burst(float x, float y, int col, int n, float sp) {
    for (int k = 0; k < n; k++)
        for (int i = 0; i < DFL_MAX_PARTS; i++) {
            DflPart *p = &dfg.part[i];
            if (p->life > 0) continue;
            float a = rng_float(&g_rng) * 6.2831853f, s = sp * (0.3f + rng_float(&g_rng) * 0.7f);
            p->x = x;
            p->y = y;
            p->vx = cosf(a) * s;
            p->vy = sinf(a) * s;
            p->life = 14 + rng_range(&g_rng, 0, 14);
            p->col = col;
            break;
        }
}

int dfl_add_eshot(int kind, float x, float y, float vx, float vy) {
    for (int i = 0; i < DFL_MAX_ESHOTS; i++) {
        DflEShot *s = &dfg.es[i];
        if (s->alive) continue;
        memset(s, 0, sizeof *s);
        s->alive = 1;
        s->kind = (uint8_t)kind;
        s->x = x;
        s->y = y;
        s->vx = vx;
        s->vy = vy;
        s->r = kind == ES_FLARE ? 3.0f : 2.0f;
        s->g = kind == ES_SHRAPNEL ? 0.06f : 0;
        return i;
    }
    return -1;
}

/* the car nearest to x that is in play (-1: none) */
int dfl_nearest_car(float x) {
    int best = -1;
    float bd = 1e9f;
    for (int p = 0; p < DFL_CARS; p++) {
        DflCar *c = &dfg.car[p];
        if (!c->on || !c->alive) continue;
        float d = fabsf(c->x - x);
        if (d < bd) { bd = d; best = p; }
    }
    return best;
}

/* n pellets fanned around the line to the nearest car */
void dfl_aimed(float x, float y, float speed, int n, float spread_deg) {
    int p = dfl_nearest_car(x);
    float tx = p >= 0 ? dfg.car[p].x : 160, ty = DFL_ROAD_Y - 6;
    float a = atan2f(ty - y, tx - x);
    for (int k = 0; k < n; k++) {
        float o = n > 1 ? (k - (n - 1) * 0.5f) * spread_deg * 3.14159265f / 180.0f : 0;
        dfl_add_eshot(ES_PELLET, x, y, cosf(a + o) * speed, sinf(a + o) * speed);
    }
}

void dfl_ring(float x, float y, float speed, int n, float rot) {
    for (int k = 0; k < n; k++) {
        float a = rot + k * 6.2831853f / n;
        dfl_add_eshot(ES_PELLET, x, y, cosf(a) * speed, sinf(a) * speed);
    }
}

void dfl_clear_world(bool keep_boss) {
    for (int i = 0; i < DFL_MAX_FOES; i++) {
        DflFoe *e = &dfg.foe[i];
        if (!e->alive) continue;
        if (keep_boss && e->kind == FK_PLANET) continue; /* part of the boss fight */
        if (DFL_FOE[e->kind].points > 0 && e->arg != DFL_ADD) dfg.escaped++;
        dfl_burst(e->x, e->y, C_WHITE, 4, 1.5f);
        e->alive = 0;
    }
    memset(dfg.es, 0, sizeof dfg.es);
}

/* ---- hits against a car --------------------------------------------------------- */

static bool car_vulnerable(int p) {
    const DflCar *c = &dfg.car[p];
    return c->on && c->alive && c->inv <= 0 && !dfg.god;
}

bool dfl_car_hit_rect(int p, float x0, float y0, float x1, float y1) {
    if (!car_vulnerable(p)) return false;
    const DflCar *c = &dfg.car[p];
    return x1 > c->x - DFL_HW && x0 < c->x + DFL_HW && y1 > DFL_HIT_TOP && y0 < DFL_ROAD_Y;
}

bool dfl_car_hit_circle(int p, float x, float y, float r) {
    if (!car_vulnerable(p)) return false;
    const DflCar *c = &dfg.car[p];
    float cx = fclamp(x, c->x - DFL_HW, c->x + DFL_HW), cy = fclamp(y, (float)DFL_HIT_TOP, (float)DFL_ROAD_Y);
    float dx = x - cx, dy = y - cy;
    return dx * dx + dy * dy < r * r;
}

void dfl_kill_car(int p, int cause) {
    DflCar *c = &dfg.car[p];
    if (!c->alive) return;
    dfg.cause = cause;
    c->alive = false;
    c->dead_t = DFL_RESPAWN_T;
    c->vx = 0;
    dfg.lost_stage++;
    dfg.deaths++;
    dfg.shake = 12;
    dfl_burst(c->x, DFL_ROAD_Y - 6, C_RED, 18, 2.6f);
    dfl_burst(c->x, DFL_ROAD_Y - 6, C_YELLOW, 14, 2.0f);
    sfx_play_name("dfl_crash");
}

/* a car comes (back) into play: from the reserve it jumps in fully charged */
void dfl_car_spawn(int p, bool from_left) {
    DflCar *c = &dfg.car[p];
    c->alive = true;
    c->dead_t = 0;
    c->vx = 0;
    c->aim = 0;
    c->face = 1;
    c->meter = DFL_METER_MAX;
    c->main_cd = c->side_cd = 0;
    c->drifting = false;
    if (from_left) {
        c->x = -14;
        c->enter = 22;
        c->inv = DFL_INV_T;
    } else {
        c->x = 120.0f;
        c->enter = 0;
        c->inv = 0;
    }
}

/* a lost car's turn is up: the next car drives in from the left, and every
 * foe but the boss is swept off the screen */
static void car_comeback(int p) {
    if (dfg.spare <= 0) return; /* out of cars: this player waits */
    dfg.spare--;
    dfl_car_spawn(p, true);
    dfl_clear_world(true);
}

/* ---- a car's frame: drive, swing the gun, charge, fire ------------------------------------ */

static void fire_main(int p) {
    DflCar *c = &dfg.car[p];
    float a = c->aim * 3.14159265f / 180.0f, sp = DFL_SHOT_SPEED[dfl_tier(c->meter)];
    c->fired++;
    for (int i = 0; i < DFL_MAX_PSHOTS; i++) {
        DflPShot *s = &dfg.ps[i];
        if (s->alive) continue;
        s->alive = 1;
        s->side = 0;
        s->owner = (uint8_t)p;
        s->x = c->x + 0.5f + sinf(a) * DFL_GUN_LEN; /* the barrel's tip on the roof */
        s->y = DFL_GUN_Y - cosf(a) * DFL_GUN_LEN;
        s->vx = sinf(a) * sp;
        s->vy = -cosf(a) * sp;
        s->dmg = DFL_TIER_DMG[dfl_tier(c->meter)];
        break;
    }
    dfl_sfx("dfl_shot", 4);
}

static void fire_side(int p, int dir) {
    DflCar *c = &dfg.car[p];
    float sp = DFL_SHOT_SPEED[dfl_tier(c->meter)];
    int made = 0;
    c->pairs++;
    for (int i = 0; i < DFL_MAX_PSHOTS && made < 2; i++) {
        DflPShot *s = &dfg.ps[i];
        if (s->alive) continue;
        s->alive = 1;
        s->side = 1;
        s->owner = (uint8_t)p;
        s->x = c->x + dir * 10;
        s->dmg = DFL_TIER_DMG[dfl_tier(c->meter)];
        if (made == 0) { s->y = DFL_ROAD_Y - 5; s->vx = dir * sp; s->vy = 0; }
        else { s->y = DFL_ROAD_Y - 7; s->vx = dir * sp * 0.766f; s->vy = -sp * 0.643f; } /* the V's upper arm, 40 degrees up */
        made++;
    }
    dfl_sfx("dfl_side", 6);
}

/* in: the driver's pad; gun: the gunner's (the same pad in 1P) */
static void car_update(int p, uint16_t in, uint16_t gun, bool bonus) {
    DflCar *c = &dfg.car[p];
    c->prev = in;
    if (!c->on) return;
    if (!c->alive) {
        if (c->dead_t > 0 && --c->dead_t == 0) car_comeback(p);
        return;
    }
    if (c->inv > 0) c->inv--;
    int dir = ((in & BTN_RIGHT) ? 1 : 0) - ((in & BTN_LEFT) ? 1 : 0);
    if (c->enter > 0) {
        c->enter--;
        dir = 0;
        c->vx = DFL_SPEED;
        c->x += c->vx;
        if (c->enter == 0) c->vx = 0;
        return;
    }
    if (dir) c->vx = fapproach(c->vx, dir * DFL_SPEED, DFL_ACCEL);
    else c->vx = fapproach(c->vx, 0, DFL_FRICTION);
    c->x += c->vx;
    if (c->x < DFL_MIN_X) { c->x = DFL_MIN_X; c->vx = 0; }
    if (c->x > DFL_MAX_X) { c->x = DFL_MAX_X; c->vx = 0; }
    /* the gun swings the way you steer; with the pad still it stays where it
     * is, and UP brings it back to straight up. In 2P the gunner's pad does
     * this, and the driver's steering leaves the gun alone. */
    int adir = dfg.players == 2 ? ((gun & BTN_RIGHT) ? 1 : 0) - ((gun & BTN_LEFT) ? 1 : 0) : dir;
    if (adir) {
        c->aim = fclamp(c->aim + adir * DFL_AIM_SWING, -DFL_AIM_MAX, DFL_AIM_MAX);
        c->face = adir;
    } else if (gun & BTN_UP) {
        c->aim = fapproach(c->aim, 0, DFL_AIM_CENTRE);
    }
    /* the power drift: only driving left fills the meter */
    c->drifting = dir < 0 && c->vx <= DFL_DRIFT_VX;
    int tier0 = dfl_tier(c->meter);
    if (c->drifting) {
        c->meter = imin(DFL_METER_MAX, c->meter + DFL_FILL);
        c->drift_frames++;
        if ((dfg.frame_t & 1) == 0)
            for (int i = 0; i < DFL_MAX_PARTS; i++)
                if (dfg.part[i].life <= 0) {
                    dfg.part[i] = (DflPart){c->x + 8, DFL_ROAD_Y - 1, 0.6f + rng_float(&g_rng), -0.4f - rng_float(&g_rng),
                                           8 + rng_range(&g_rng, 0, 6), rng_chance(&g_rng, 50) ? C_YELLOW : C_ORANGE};
                    break;
                }
        dfl_sfx("dfl_drift", 9);
    } else if (!bonus) {
        c->meter = imax(0, c->meter - DFL_DRAIN);
    }
    if (dfl_tier(c->meter) > tier0) sfx_play_name("dfl_tier");
    if (c->main_cd > 0) c->main_cd--;
    if (c->side_cd > 0) c->side_cd--;
    int tier = dfl_tier(c->meter);
    if ((gun & DFL_BTN_MAIN) && c->main_cd == 0) {
        fire_main(p);
        c->main_cd = DFL_MAIN_CD[tier];
    }
    if ((gun & DFL_BTN_SIDE) && c->side_cd == 0) {
        fire_side(p, adir ? adir : c->face);
        c->side_cd = DFL_SIDE_CD[tier];
    }
}

/* ---- shots ------------------------------------------------------------------------ */

static void pshots_update(void) {
    for (int i = 0; i < DFL_MAX_PSHOTS; i++) {
        DflPShot *s = &dfg.ps[i];
        if (!s->alive) continue;
        s->x += s->vx;
        s->y += s->vy;
        if (s->x < -8 || s->x > SCREEN_W + 8 || s->y < DFL_HUD_H - 4 || s->y > SCREEN_H) { s->alive = 0; continue; }
        if (dfg.state == DS_BONUS) continue; /* blocks are handled by the bonus stage */
        if (dfg.boss.on && !dfg.boss.dead && dfl_boss_shot(s)) { s->alive = 0; continue; }
        for (int k = 0; k < DFL_MAX_FOES; k++) {
            DflFoe *e = &dfg.foe[k];
            if (!e->alive) continue;
            const DflFoeDef *d = &DFL_FOE[e->kind];
            if (fabsf(s->x - e->x) > d->hw + 2 || fabsf(s->y - e->y) > d->hh + 2) continue;
            /* a ghost fading through can't be hit; shots go through it */
            if (e->kind == FK_SHEET && e->state == 2) continue;
            dfg.last_hit_dmg = s->dmg;
            dfl_hurt_foe(k, s->dmg, s->side, s->vx);
            s->alive = 0;
            break;
        }
    }
}

static void eshots_update(void) {
    for (int i = 0; i < DFL_MAX_ESHOTS; i++) {
        DflEShot *s = &dfg.es[i];
        if (!s->alive) continue;
        s->t++;
        s->vy += s->g;
        s->x += s->vx;
        s->y += s->vy;
        if (s->x < -10 || s->x > SCREEN_W + 10 || s->y < -10 || s->y > DFL_ROAD_BOT + 2) {
            s->alive = 0;
            continue;
        }
        for (int p = 0; p < DFL_CARS; p++)
            if (dfl_car_hit_circle(p, s->x, s->y, s->r)) {
                s->alive = 0;
                dfl_kill_car(p, CAUSE_SHOT + s->kind);
                break;
            }
    }
}

static void parts_update(void) {
    for (int i = 0; i < DFL_MAX_PARTS; i++) {
        DflPart *p = &dfg.part[i];
        if (p->life <= 0) continue;
        p->life--;
        p->x += p->vx;
        p->y += p->vy;
        p->vy += 0.05f;
    }
}

/* contact with foes */
static void touches(void) {
    for (int k = 0; k < DFL_MAX_FOES; k++) {
        DflFoe *e = &dfg.foe[k];
        if (!e->alive || e->t < 0) continue;
        if (e->kind == FK_SHEET && e->state == 2) continue;
        const DflFoeDef *d = &DFL_FOE[e->kind];
        for (int p = 0; p < DFL_CARS; p++)
            if (dfl_car_hit_rect(p, e->x - d->hw + 1, e->y - d->hh + 1, e->x + d->hw - 1, e->y + d->hh - 1)) dfl_kill_car(p, CAUSE_FOE + e->kind);
    }
}

/* ---- the run ------------------------------------------------------------------------ */

void dfl_new_run(int players) {
    dfg.players = players;
    dfg.score = 0;
    dfg.spare = DFL_START_SPARE;
    dfg.most_cars = dfg.spare;
    dfg.deaths = 0;
    dfg.coins = 0;
    dfg.bonus_played = 0;
    dfg.kills = 0;
    dfg.escaped = 0;
    dfg.won = false;
    dfg.god = false;
    memset(dfg.car, 0, sizeof dfg.car);
    /* one car, in 2P too: one player drives, the other mans the gun */
    dfg.car[0].on = true;
    dfg.car[0].col = C_RED;
    dfl_car_spawn(0, false);
    dfg.car[0].meter = 0; /* the first car of a run starts with the meter empty */
    if (dfs.runs < 65535) dfs.runs++;
    dfl_save_now();
    dfl_start_stage(0);
}

void dfl_start_stage(int s) {
    if (s == 0) { dfg.hog_hinted = false; dfg.hog_hint_t = 0; }
    dfg.stage = s;
    dfg.stage_t = 0;
    dfg.spawn_i = 0;
    dfg.scroll = 0;
    dfg.swell_i = 0;
    dfg.swell_loop = 0;
    memset(dfg.swell, 0, sizeof dfg.swell);
    memset(&dfg.boss, 0, sizeof dfg.boss);
    memset(dfg.foe, 0, sizeof dfg.foe);
    memset(dfg.es, 0, sizeof dfg.es);
    memset(dfg.ps, 0, sizeof dfg.ps);
    dfg.lost_stage = 0;
    dfg.kills_stage = 0;
    dfg.foes_stage = 0;
    dfg.stage_points = 0;
    dfg.clear_t = 0;
    for (int p = 0; p < DFL_CARS; p++) {
        DflCar *c = &dfg.car[p];
        if (!c->on) continue;
        if (c->alive) {
            c->x = 120.0f;
            c->vx = 0;
            c->enter = 0;
            c->inv = 0;
        } else if (dfg.spare > 0) {
            dfg.spare--;
            dfl_car_spawn(p, false);
        }
    }
    if (s + 1 > dfs.best_stage) dfs.best_stage = (uint8_t)(s + 1);
}

/* one frame of a stage; the shell's state machine reads what happened */
void dfl_play_update(uint16_t in0, uint16_t in1) {
    dfg.frame_t++;
    if (dfg.hog_hint_t > 0) dfg.hog_hint_t--;
    dfg.stage_t++;
    dfg.scroll += DFL_SCROLL;
    if (dfg.shake > 0) dfg.shake--;
    car_update(0, in0, dfg.players == 2 ? in1 : in0, false);
    if (!dfg.boss.on) dfl_run_spawns();
    if (!dfg.boss.on && dfg.stage_t >= DFL_STAGE[dfg.stage].boss_t) dfl_boss_start(dfg.stage);
    dfl_foes_update();
    if (dfg.boss.on) dfl_boss_update();
    dfl_swells_update();
    pshots_update();
    eshots_update();
    touches();
    parts_update();
    dfl_note_cars();
}

/* ---- the bonus stage: the car is the paddle, the coin in its bubble the ball -------- */

#define BALL_R 6.0f
#define BALL_SPEED 2.5f
#define BONUS_LEFT 8.0f
#define BONUS_RIGHT 312.0f
#define BONUS_TOP (DFL_HUD_H + 2.0f)

void dfl_bonus_start(void) {
    int m = iclamp(dfg.stage, 0, 2);
    dfg.blocks_left = dfg.blocks_total = dfg.blocks_broken = dfg.blocks_shot = 0;
    dfg.bonus_points = 0;
    for (int r = 0; r < DFL_BROWS; r++)
        for (int c = 0; c < DFL_BCOLS; c++) {
            bool on = DFL_BONUS_MAP[m][r][c] == '#';
            dfg.block[r][c] = on ? DFL_BLOCK_HP : 0;
            dfg.blocks_left += on;
        }
    dfg.blocks_total = dfg.blocks_left;
    dfg.bx = 160;
    dfg.by = 104;
    dfg.bvx = 0.9f;
    dfg.bvy = 2.33f;
    dfg.ball_live = true;
    dfg.bonus_won = false;
    dfg.coin_out = false;
    dfg.coin_y = 0;
    dfg.bonus_played++;
    memset(dfg.foe, 0, sizeof dfg.foe);
    memset(dfg.es, 0, sizeof dfg.es);
    memset(dfg.ps, 0, sizeof dfg.ps);
    memset(&dfg.boss, 0, sizeof dfg.boss);
    memset(dfg.swell, 0, sizeof dfg.swell);
    for (int p = 0; p < DFL_CARS; p++) {
        DflCar *c = &dfg.car[p];
        if (!c->on) continue;
        if (!c->alive && dfg.spare > 0) { dfg.spare--; dfl_car_spawn(p, false); }
        c->x = 160.0f;
        c->vx = 0;
        c->enter = 0;
        c->inv = 0;
    }
}

/* the block at a point (-1: none) */
static int block_at(float x, float y, int *rr, int *cc) {
    int c = (int)floorf((x - DFL_BLOCK_X0) / DFL_BLOCK_W), r = (int)floorf((y - DFL_BLOCK_Y0) / DFL_BLOCK_H);
    if (c < 0 || c >= DFL_BCOLS || r < 0 || r >= DFL_BROWS || !dfg.block[r][c]) return -1;
    *rr = r;
    *cc = c;
    return 1;
}

static void break_block(int r, int c, bool shot) {
    dfg.block[r][c] = 0;
    dfg.blocks_left--;
    uint32_t pts = 1000u + 200u * (uint32_t)dfg.blocks_broken;
    if (shot) { pts += 800u; dfg.blocks_shot++; }
    dfg.blocks_broken++;
    dfg.bonus_points += pts;
    dfl_add_score(pts);
    float x = DFL_BLOCK_X0 + c * DFL_BLOCK_W + DFL_BLOCK_W / 2.0f, y = DFL_BLOCK_Y0 + r * DFL_BLOCK_H + DFL_BLOCK_H / 2.0f;
    dfl_burst(x, y, shot ? C_YELLOW : C_PINK, 8, 1.8f);
    dfl_sfx("dfl_block", 2);
    if (dfg.blocks_left <= 0) {
        dfg.bonus_won = true;
        dfg.ball_live = false;
        dfg.coin_out = true;
        dfg.coin_y = dfg.by;
        sfx_play_name("dfl_pop");
    }
}

static void ball_step(float f) {
    dfg.bx += dfg.bvx * f;
    dfg.by += dfg.bvy * f;
    if (dfg.bx < BONUS_LEFT + BALL_R) { dfg.bx = BONUS_LEFT + BALL_R; dfg.bvx = fabsf(dfg.bvx); dfl_sfx("dfl_bounce", 3); }
    if (dfg.bx > BONUS_RIGHT - BALL_R) { dfg.bx = BONUS_RIGHT - BALL_R; dfg.bvx = -fabsf(dfg.bvx); dfl_sfx("dfl_bounce", 3); }
    if (dfg.by < BONUS_TOP + BALL_R) { dfg.by = BONUS_TOP + BALL_R; dfg.bvy = fabsf(dfg.bvy); dfl_sfx("dfl_bounce", 3); }
    int r, c;
    /* the blocks: one hit breaks one, and the coin bounces off it */
    if (block_at(dfg.bx + (dfg.bvx > 0 ? BALL_R : -BALL_R), dfg.by, &r, &c) > 0) {
        dfg.bvx = -dfg.bvx;
        break_block(r, c, false);
        if (!dfg.ball_live) return;
    }
    if (block_at(dfg.bx, dfg.by + (dfg.bvy > 0 ? BALL_R : -BALL_R), &r, &c) > 0) {
        dfg.bvy = -dfg.bvy;
        break_block(r, c, false);
        if (!dfg.ball_live) return;
    }
    /* the cars */
    if (dfg.bvy > 0 && dfg.by + BALL_R >= DFL_CAR_TOP && dfg.by + BALL_R <= DFL_CAR_TOP + 8) {
        for (int p = 0; p < DFL_CARS; p++) {
            DflCar *car = &dfg.car[p];
            if (!car->on || !car->alive) continue;
            float off = (dfg.bx - car->x) / 15.0f;
            if (fabsf(off) > 1.0f) continue;
            float a = off * 60.0f * 3.14159265f / 180.0f;
            dfg.bvx = sinf(a) * BALL_SPEED;
            dfg.bvy = -cosf(a) * BALL_SPEED;
            dfg.by = DFL_CAR_TOP - BALL_R;
            dfl_sfx("dfl_paddle", 3);
            break;
        }
    }
    /* never too flat */
    float minvy = BALL_SPEED * 0.4f;
    if (fabsf(dfg.bvy) < minvy) {
        dfg.bvy = dfg.bvy < 0 ? -minvy : minvy;
        float vx2 = BALL_SPEED * BALL_SPEED - minvy * minvy;
        dfg.bvx = (dfg.bvx < 0 ? -1.0f : 1.0f) * sqrtf(vx2);
    }
}

void dfl_bonus_update(uint16_t in0, uint16_t in1) {
    dfg.frame_t++;
    dfg.stage_t++;
    dfg.scroll += DFL_SCROLL * 0.5f;
    car_update(0, in0, dfg.players == 2 ? in1 : in0, true);
    /* gunfire chips at the blocks; enough of it breaks one (worth 800 more) */
    for (int i = 0; i < DFL_MAX_PSHOTS; i++) {
        DflPShot *s = &dfg.ps[i];
        if (!s->alive) continue;
        int r, c;
        if (block_at(s->x, s->y, &r, &c) > 0) {
            s->alive = 0;
            dfg.last_hit_dmg = s->dmg;
            if (dfg.block[r][c] <= s->dmg) break_block(r, c, true);
            else { dfg.block[r][c] = (uint8_t)(dfg.block[r][c] - s->dmg); dfl_sfx("dfl_chip", 3); }
        }
    }
    pshots_update();
    if (dfg.ball_live && dfg.stage_t > 60) {
        ball_step(0.5f);
        if (dfg.ball_live) ball_step(0.5f);
        if (dfg.ball_live && dfg.by - BALL_R > SCREEN_H) {
            dfg.ball_live = false;
            sfx_play_name("dfl_lost");
        }
    }
    if (dfg.coin_out) {
        /* the bubble is open: the coin drops into the car */
        dfg.coin_y += 1.6f;
        if (dfg.coin_y >= DFL_CAR_TOP - 4) {
            dfg.coin_out = false;
            dfg.coins++;
            dfg.spare += 2;
            dfl_note_cars();
            if (dfs.coins < 65535) dfs.coins++;
            game_award(GOAL_BEACON);
            sfx_play_name("dfl_coin");
            music_play(DFL_MUS_COIN);
        }
    }
    parts_update();
}

void dfl_save_now(void) { game_save_write(game_current_index(), &dfs, (int)sizeof dfs); }
